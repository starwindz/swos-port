#pragma once

class ProcHookList
{
public:
    static constexpr size_t kProcNameLength = 256;

    void add(const String& procName, const String& hookName, int line, int definedAtLine);

    struct ProcHookItem {
        ProcHookItem(const String& procName, const String& hookName, int line, int nextIndex, int definedAtLine)
            : procName(procName), hookName(hookName), line(line), nextIndex(nextIndex), definedAtLine(definedAtLine) {}
        String procName;
        String hookName;
        int line;
        int definedAtLine;
        int nextIndex;
    };

    std::vector<ProcHookItem> getItems();
    String encodeProcHook(const ProcHookItem *begin, const ProcHookItem *end);

    static int getCurrentHookLine(String& procHook);
    static String getCurrentHookProc(String& procHook);
    static bool moveToNextHook(String& procHook);

private:
#pragma pack(push, 1)
    class PackedProcHookList {
    public:
        PackedProcHookList(const ProcHookItem *begin, const ProcHookItem *end, size_t size);
        static size_t requiredSize(const ProcHookItem *begin, const ProcHookItem *end);
        bool moveToNextHook();
        int getCurrentLine() const;
        String getCurrentHookProc() const;

    private:
        struct HookHeader {
            size_t size() const {
                return sizeof(*this) + hookNameLen;
            }
            HookHeader *next() const {
                return reinterpret_cast<HookHeader *>((char *)this + size());
            }
            String hookName() const {
                return { (char *)(this + 1), hookNameLen };
            }

            uint32_t line;
            uint32_t hookNameLen;
        };

        char *m_sentinel;
        union {
            HookHeader *m_currentHook;
            char *m_currentHookCharPtr;
        };
        uint8_t m_procNameLen;
    };
#pragma pack(pop)

    struct ProcHookItemInternal {
        char procName[kProcNameLength];
        char hookName[kProcNameLength];
        size_t procNameLen;
        size_t hookNameLen;
        int line;
        int definedAtLine;
        int lastIndex = -1;
        bool first = false;
    };

    std::vector<ProcHookItemInternal *> getSortedItems();

    std::vector<ProcHookItemInternal> m_items;
    std::vector<ProcHookItemInternal *> m_sortedItems;
};
