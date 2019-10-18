#include "AsmConverter.h"
#include "AsmConverterWorker.h"
#include "Tokenizer.h"
#include "IdaAsmParser.h"
#include "Util.h"

constexpr int kDefinesCapacity = 45'000;
constexpr auto kDefsFilename = "defs.inc";

AsmConverter::AsmConverter(const char *inputPath, const char *outputPath, const char *symbolsFile,
    const char *swosHeaderFile, const char *format, int numFiles)
:
    m_inputPath(inputPath), m_outputPath(outputPath), m_headerPath(swosHeaderFile), m_format(format),
    m_numFiles(numFiles), m_symFileParser(symbolsFile, swosHeaderFile), m_defines(kDefinesCapacity)
{
    if (!OutputFactory::formatSupported(format))
        Util::exit("Unsupported format: %s, supported formats: %s", 1, format, OutputFactory::getSupportedFormats().c_str());

    loadFile(inputPath);
}

void AsmConverter::convert()
{
    String commonPrefix;
    const char *codeStart;
    std::tie(codeStart, commonPrefix) = findCodeDataStart();

    int commonPartLength = codeStart - m_data.get();
    int remainingLength = m_dataLength - commonPartLength;

    int blockSize = remainingLength / m_numFiles;

    auto lineNo = parse(commonPartLength, blockSize);

    checkForParsingErrors(lineNo);
    auto workersToOutput = connectRanges();
    resolveExterns(workersToOutput);
    output(commonPrefix, workersToOutput);

    checkForUnusedSymbols();
}

void AsmConverter::loadFile(const char *path)
{
    // make sure new line is guaranteed to end the file, so the tokenizer can be a bit simpler
    auto result = Util::loadFile(path, true);
    m_data.reset(result.first);
    m_dataLength = result.second;

    if (!m_dataLength)
        Util::exit("Could not process file %s, it seems to be empty", EXIT_FAILURE, path);
}

std::pair<const char *, String> AsmConverter::findCodeDataStart() const
{
    // skip structs, defines and intro comments
    auto start = strstr(m_data.get(), ".586");
    assert(start);
    auto p = start;

    while (*p != ';')
        p++;

    auto end = p - 1;
    while (Util::isSpace(*end))
        end--;

    while (*end != '\n')
        end++;

    String commonPrefix(start, end - start + 1);

    return { p, commonPrefix };
}

size_t AsmConverter::parseCommonPart(int length)
{
    Tokenizer tokenizer;
    auto limits = tokenizer.tokenize(m_data.get(), length);

    auto symbolTableCopy = new SymbolTable(m_symFileParser.symbolTable());
    IdaAsmParser parser(m_symFileParser, *symbolTableCopy, tokenizer, m_structs, m_defines);
    parser.parse();

    if (!parser.ok())
        error(parser.errorDescription(), parser.errorLine());

    return parser.lineCount();
}

size_t AsmConverter::parse(int commonPartLength, int blockSize)
{
    for (int i = 0; i < m_numFiles; i++) {
        m_symbolTables.push_back(new SymbolTable(m_symFileParser.symbolTable()));
        int chunkOffset = commonPartLength - Util::kNewLine.size() + i * blockSize;

        m_workers.push_back(new AsmConverterWorker(i + 1, m_data.get(), m_dataLength,
            chunkOffset, blockSize, m_symFileParser, *m_symbolTables.back()));

        auto future = std::async(std::launch::async, &AsmConverterWorker::process, m_workers.back());
        m_futures.push_back(std::move(future));
    }

    // utilize main thread too, this should finish first
    auto lineNo = parseCommonPart(commonPartLength);

    // workers should still be busy, so we basically we get this for free :)
    m_symFileParser.outputHeaderFile(m_headerPath);

    waitForWorkers();

    return lineNo;
}

void AsmConverter::checkForParsingErrors(size_t lineNo)
{
    for (auto worker : m_workers) {
        const auto& parser = worker->parser();

        if (!parser.ok()) {
            auto errorLine = lineNo + parser.errorLine();
            error(parser.errorDescription(), errorLine);
        }
        lineNo += parser.lineCount();
    }
}

std::vector<int> AsmConverter::connectRanges()
{
    std::vector<int> activeChunks(m_numFiles, false);
    String missingSymbol;

    for (int i = 0; i < m_numFiles - 1; i++) {
        const auto& parser = m_workers[i]->parser();

        if (missingSymbol.empty()) {
            assert(parser.foundEndRangeSymbol().empty());

            activeChunks[i] = true;
            missingSymbol = parser.missingEndRangeSymbol();
        } else {
            if (missingSymbol == parser.foundEndRangeSymbol()) {
                missingSymbol.clear();
                activeChunks[i] = true;
            } else {
                assert(parser.foundEndRangeSymbol().empty());
            }
        }
    }

    assert(m_workers.back()->parser().missingEndRangeSymbol().empty());
    activeChunks.back() = true;

    return activeChunks;
}

void AsmConverter::resolveExterns(const AllowedChunkList& allowedChunks)
{
    collectSegments();

    auto constWorkers = std::vector<const AsmConverterWorker *>{ m_workers.begin(), m_workers.end() };

    for (size_t i = 0; i < m_futures.size(); i++) {
        if (allowedChunks[i]) {
            auto future = std::async(std::launch::async, &AsmConverterWorker::resolveReferences, m_workers[i],
                std::ref(constWorkers), std::ref(m_segments), std::ref(m_structs), std::ref(m_defines));
            m_futures[i] = std::move(future);
        }
    }

    waitForWorkers();
}

void AsmConverter::collectSegments()
{
    for (auto worker : m_workers) {
        for (const auto& segmentRange : worker->parser().segments()) {
            assert(segmentRange.first->isId() && segmentRange.first->textLength);
            m_segments.add(segmentRange);
        }
    }
}

void AsmConverter::output(const String& commonPrefix, const AllowedChunkList& activeChunks)
{
    assert(!m_segments.empty());

    // put 1st chunk externs in the same segment to avoid implicit far jump/call error
    CToken *openSegment = m_workers[0]->parser().firstSegment();
    CToken *externDefSegment{};
    std::string prefix = commonPrefix.string() + "include " + kDefsFilename + Util::kNewLineString() + Util::kNewLineString();

    for (size_t i = 0; i < m_futures.size(); i++) {
        if (!activeChunks[i])
            continue;

        CToken *lastSegment{};
        if (i > 0) {
            lastSegment = m_workers[i - 1]->parser().currentSegment();

            if (lastSegment)
                openSegment = lastSegment;

            externDefSegment = nullptr;
        } else {
            externDefSegment = openSegment;
        }

        m_workers[i]->setCImportSymbols(m_symFileParser.imports());
        m_workers[i]->setCExportSymbols(m_symFileParser.exports());

        auto future = std::async(std::launch::async, &AsmConverterWorker::output, m_workers[i], m_format, m_outputPath,
            std::ref(m_structs), std::ref(m_defines), std::cref(prefix), std::make_pair(openSegment, i == 0));
        m_futures[i] = std::move(future);
    }

    outputStructsAndDefines();

    waitForWorkers();

    checkForOutputErrors();
}

void AsmConverter::checkForOutputErrors()
{
    bool failed = false;

    for (auto worker : m_workers) {
        if (!worker->outputOk()) {
            std::cout << "Error in output file " << worker->filename() << ":\n" << worker->outputError() << '\n';
            failed = true;
        }
    }

    assert(m_numFiles == m_workers.size() && m_numFiles == m_futures.size());

    for (int i = 0; i < m_numFiles - 1; i++) {
        const auto& limitsError = m_workers[i]->limitsError();
        if (!limitsError.empty()) {
            std::cout << "Check limits of file " << m_workers[i]->filename() <<
                "! Possible loss of data at beginning and/or end: " << limitsError << '\n';
            failed = true;
        }
    }

    for (size_t i = 1; i < m_workers.size(); i++) {
        bool prevNoBreakContinued, prevNoBreakOverflow;
        bool noBreakContinued, noBreakOverflow;

        std::tie(prevNoBreakContinued, prevNoBreakOverflow) = m_workers[i - 1]->noBreakTagState();
        std::tie(noBreakContinued, noBreakOverflow) = m_workers[i]->noBreakTagState();

        if (!prevNoBreakOverflow && noBreakContinued) {
            std::cout << "Unexpected closing no break tag found in " << m_workers[i]->filename() << "!\n";
            failed = true;
        }
    }

    if (failed)
        std::exit(EXIT_FAILURE);
}

void AsmConverter::checkForUnusedSymbols()
{
    auto exitIfUndefinedSymbols = [](const char *lead, const std::vector<String>& symbols) {
        if (!symbols.empty()) {
            std::string error = lead;

            for (const auto& str : symbols)
                error += str.string() + ", ";

            error.erase(error.length() - 2, 2);
            Util::exit("%s", EXIT_FAILURE, error.c_str());
        }
    };

    assert(m_symbolTables.size() == m_workers.size() && m_symbolTables[0]);

    for (size_t i = 1; i < m_symbolTables.size(); i++)
        m_symbolTables.front()->mergeSymbols(*m_symbolTables[i]);

    auto possiblyUnusedSymbols = m_symbolTables.front()->unusedSymbolsForRemoval();
    for (const auto& exp : m_symFileParser.exports())
        possiblyUnusedSymbols.push_back(exp);

    if (!possiblyUnusedSymbols.empty()) {
        decltype(possiblyUnusedSymbols) unusedSymbols;

        for (const auto& sym : possiblyUnusedSymbols) {
            auto it = std::find_if(m_workers.begin(), m_workers.end(), [&sym](const auto worker) {
                const auto& refs = worker->parser().references();
                return refs.hasReference(sym) || refs.hasPublic(sym);
            });

            if (it == m_workers.end())
                unusedSymbols.push_back(sym);
        }

        exitIfUndefinedSymbols("Unknown symbol(s) found: ", unusedSymbols);
    }
}

std::string AsmConverter::formOutputFilename() const
{
    auto outputFilePath = Util::getBasePath(m_outputPath);

    if (outputFilePath.back() != '\\')
        outputFilePath += '\\';
    outputFilePath += kDefsFilename;

    return outputFilePath;
}

void AsmConverter::outputStructsAndDefines()
{
    auto outputFilePath = formOutputFilename();
    m_outputWriter = OutputFactory::create(m_format, outputFilePath.c_str(), m_symFileParser, m_structs, m_defines,
        References(), OutputItemStream());

    if (!m_outputWriter->output(OutputWriter::kStructs | OutputWriter::kDefines))
        Util::exit("Error writing %s.", EXIT_FAILURE, Util::getFilename(outputFilePath.c_str()));
}

void AsmConverter::waitForWorkers()
{
    for (auto& future : m_futures)
        future.wait();
}

void AsmConverter::error(const std::string& desc, size_t lineNo)
{
    Util::exit("%s(%d): %s.", EXIT_FAILURE, m_inputPath, lineNo, desc.c_str());
}
