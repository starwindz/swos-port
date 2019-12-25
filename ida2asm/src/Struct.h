#pragma once

#include "Iterator.h"
#include "Tokenizer.h"
#include "DynaArray.h"

#pragma pack(push, 1)
class Struct {
public:
    Struct(CToken *name, const TokenList& leadingComments, CToken *comment, bool isUnion, size_t size);
    static size_t requiredSize(CToken *name, const TokenList& leadingComments, CToken *comment);

    String name() const;
    Util::hash_t hash() const;
    String leadingComments() const;
    String lineComment() const;
    bool isUnion() const;

    bool resolved() const;
    void resolve();
    bool dupNotAllowed() const;
    void disallowDup();

    Struct *next() const;
    void increaseSize(uint32_t increment);

    bool operator==(const String& str) const;

    struct Field {
        Field(CToken *name, size_t elementSize, CToken *comment, CToken *type, CToken *dup) {
            m_size = requiredSize(name, comment, type, dup);
            Util::assignSize(m_elementSize, elementSize);

            constexpr size_t kMax = std::numeric_limits<decltype(m_nameLength)>::max();

            const std::tuple<CToken *, uint8_t *, char * (Field::*)() const> kStringData[] = {
                { name, &m_nameLength, &Field::namePtr, },
                { type, &m_typeLength, &Field::typePtr, },
                { dup, &m_dupValueLength, &Field::dupPtr, },
                { comment, &m_commentLength, &Field::commentPtr, },
            };

            for (const auto& data : kStringData) {
                auto token = std::get<0>(data);
                auto destLength = std::get<1>(data);
                auto getBufferFunc = std::get<2>(data);

                *destLength = 0;
                if (token) {
                    assert(token->textLength <= kMax);
                    *destLength = token->textLength;
                    token->copyText((this->*getBufferFunc)());
                }
            }
        }
        static size_t requiredSize(CToken *name, CToken *comment, CToken *type, CToken *dup) {
            auto nameLength = name ? name->textLength : 0;
            auto commentLength = comment ? comment->textLength : 0;
            auto typeLength = type ? type->textLength : 0;
            auto dupLength = dup ? dup->textLength : 0;
            return sizeof(Field) + nameLength + commentLength + typeLength + dupLength;
        }

        size_t elementSize() const { return m_elementSize; }
        size_t byteSize() const { return m_elementSize * (!dup().empty() ? dup().toInt() : 1); }
        Field *next() const { return reinterpret_cast<Field *>((char *)this + m_size); }
        String name() const { return { namePtr(), m_nameLength }; }
        String comment() const { return { commentPtr(), m_commentLength }; }
        String type() const { return { typePtr(), m_typeLength }; }
        String dup() const { return { dupPtr(), m_dupValueLength }; }

    private:
        char *namePtr() const { return (char *)(this + 1); }
        char *typePtr() const { return namePtr() + m_nameLength; }
        char *dupPtr() const { return typePtr() + m_typeLength; }
        char *commentPtr() const { return dupPtr() + m_dupValueLength; }

        uint32_t m_size;
        uint8_t m_elementSize;
        uint8_t m_dupValueLength;
        uint8_t m_typeLength;
        uint8_t m_nameLength;
        uint8_t m_commentLength;
        // followed by name, type string, dup string, comment

        friend class Struct;
    };

    Field *addField(CToken *name, size_t fieldLenght, CToken *comment, CToken *type, size_t dup);
    void addComment(const String& comment, Field *lastField);
    Iterator::Iterator<Field> begin() const;
    Iterator::Iterator<Field> end() const;

private:
    char *namePtr() const;
    char *leadingCommentsPtr() const;
    char *lineCommentPtr() const;

    enum Flags : uint8_t { kNone, kResolved = 1, kCantBeDuped = 2, };

    Util::hash_t m_hash;
    uint8_t m_nameLength;
    uint8_t m_lineCommentLength;
    uint8_t m_leadingCommentsLength;
    uint8_t m_isUnion;
    uint16_t m_size;
    Flags m_flags = kNone;
    // followed by: name bytes, leading comments string, line comment string
    //   for each element: size, name length, comment length, name, comment
};
#pragma pack(pop)

class StructStream {
public:
    StructStream();
    size_t count() const;
    size_t size() const;
    bool empty() const;
    void addStruct(CToken *name, const TokenList& leadingComments, CToken *lineComment, bool isUnion);
    void addField(CToken *name, size_t fieldLenght, CToken *comment, CToken *type, CToken *dup);
    void addComment(CToken *token);
    void disallowDup();
    String lastStructName() const;
    Struct *findStruct(const String& name) const;
    Iterator::Iterator<Struct> begin() const;
    Iterator::Iterator<Struct> end() const;

private:
    size_t m_count = 0;
    DynaArray m_structs;
    Struct *m_lastStruct = nullptr;
    Struct::Field *m_lastField = nullptr;
};
