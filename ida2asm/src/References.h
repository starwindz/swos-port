#pragma once

#include "StringMap.h"
#include "Tokenizer.h"
#include "AmigaRegs.h"
#include "Util.h"

class StructStream;
class SegmentSet;

class References
{
public:
    enum ReferenceType : uint8_t {
        kNone, kNear, kProc, kByte, kWord, kDword, kQWord, kTbyte, kUser, kUnknown, kIgnore,
    };

    References();

    void addVariable(CToken *var, int size, CToken *structName);
    void addProc(CToken *proc);
    void addLabel(CToken *label);
    void addReference(CToken *ref);
    void addReference(const String& str, CToken *refProc);
    void markImport(const String& str);
    void markExport(const String& str);
    bool hasReference(const String& str) const;
    bool hasPublic(const String& str) const;
    std::pair<ReferenceType, String> getType(const String& str) const;

    void setIgnored(const String& str, Util::hash_t hash);

    void clear();
    void seal();
    void resolve();
    void resolve(const References& rhs);
    void resolveSegments(const SegmentSet& segments);

    std::vector<String> publics() const;
    std::vector<std::tuple<String, ReferenceType, String, String>> externs() const;

private:
    static bool isReference(const String& str);

    // optimize for the special case, they're extremely heavily used
    bool addAmigaRegister(CToken *reference);

#pragma pack(push, 1)
    struct RefHolder {
        RefHolder(ReferenceType type = kNone, CToken *structName = nullptr, CToken *refProc = nullptr) : type(type) {
            if (type == kUser) {
                assert(structName);
                Util::assignSize(*structNameLength(), structName->textLength);
                structName->copyText(structNamePtr());
            } else {
                Util::assignSize(*structNameLength(), 0);
            }
            if (refProc) {
                Util::assignSize(*refProcNameLength(), refProc->textLength);
                refProc->copyText(refProcNamePtr());
            } else {
                Util::assignSize(*refProcNameLength(), 0);
            }
        }
        using LengthType = uint8_t;
        static size_t requiredSize(ReferenceType type = kNone, CToken *structName = nullptr, CToken *refProc = nullptr) {
            return sizeof(RefHolder) + 2 * sizeof(LengthType) + (type == kUser ? structName->textLength : 0) + (refProc ? refProc->textLength : 0);
        }
        String structName() const {
            if (type == kUser) {
                if (structPtr)
                    return { reinterpret_cast<char *>(structPtr + 1),  *structPtr };
                else
                    return { structNamePtr(), *structNameLength() };
            }
            return String();
        }
        String refProcName() const {
            return { refProcNamePtr(), *refProcNameLength() };
        }
        LengthType *structNameLength() const { return (LengthType *)(this + 1); }
        char *structNamePtr() const { return reinterpret_cast<char *>(structNameLength()) + sizeof(LengthType); }
        LengthType *refProcNameLength() const { return reinterpret_cast<LengthType *>(structNamePtr() + *structNameLength()); }
        char *refProcNamePtr() const { return reinterpret_cast<char *>(refProcNameLength()) + sizeof(LengthType); }

        uint8_t *structPtr = nullptr;
        ReferenceType type;
        uint8_t pub = 0;
    };
#pragma pack(pop)

    std::array<bool, kNumAmigaRegisters> m_amigaRegisters = {};
    StringMap<RefHolder> m_labels;
    StringMap<RefHolder> m_references;
};
