#include "EndProc.h"
#include "Util.h"

EndProc::EndProc(CToken *name)
{
    assert(name && name->textLength);

    Util::assignSize(m_nameLength, name->textLength);
    name->copyText(namePtr());
}

String EndProc::name() const
{
    return { namePtr(), m_nameLength };
}

char *EndProc::namePtr() const
{
    return (char *)(this + 1);
}

size_t EndProc::requiredSize(CToken *name)
{
    assert(name);
    return sizeof(EndProc) + name->textLength;
}
