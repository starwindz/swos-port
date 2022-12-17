#include "JoypadsTest.h"
#include "mockLog.h"
#include "sdlProcs.h"
#include "joypads.h"
#include "util.h"

namespace SWOS_UnitTest {
    namespace Detail {
        static inline std::string stringify(const JoypadConfig::AxisInterval& interval)
        {
            return "("s + std::to_string(interval.from) + ".." + std::to_string(interval.to) + ')';
        }
    }
}

#include "unitTest.h"

static JoypadsTest t;

struct IntervalTest
{
    const char *input;
    const JoypadConfig::AxisIntervalList expected;
};

IntervalTest kIntervalTestData[] = {
    {
        "0[10:15:1,15:20:1,5:10:1,0:5:1]",
        { { 0, 5, kGameEventUp }, { 5, 10, kGameEventUp }, { 10, 15, kGameEventUp }, { 15, 20, kGameEventUp } }
    }, {
        "0[10:15:1,15:20:1,-5:10:1,-5:0:1]",
        { { -5, 10, kGameEventUp }, { 10, 15, kGameEventUp }, { 15, 20, kGameEventUp } }
    }, {
        "0[0:5:1,1:2:1,2:4:1,2:8:1,4:7:1,6:10:1,11:20:1]",
        { { 0, 5, kGameEventUp }, { 5, 8, kGameEventUp }, { 8, 10, kGameEventUp }, { 11, 20, kGameEventUp } }
    }, {
        "0[0:10:1,2:7:1,3:8:1,5:12:1,5:8:1,5:7:1,12:20:1,13:19:1,19:21:1]",
        { { 0, 10, kGameEventUp }, { 10, 12, kGameEventUp }, { 12, 20, kGameEventUp }, { 20, 21, kGameEventUp } },
    },
};

struct NamingTest
{
    FakeJoypadList initial;
    std::vector<int> toDisconnect;
    FakeJoypadList toConnect;
    std::vector<int> expectedSuffixes;
};

// keep disconnect list sorted
NamingTest kNamingTestData[] = {
    { {}, {}, { { "Total Annihilation" }, { "Total Annihilation" } }, { 1, 2 } },
    { { { "Space" }, { "Jam" }, { "Space" } }, {}, { { "Space" } }, { 1, -1, 2, 3 }, },
    { { { "Rage" }, { "Hard" }, { "Die" }, { "Hard" }, { "Road" }, { "Rage" } },
        { 1, 3 }, { { "Rage" }, { "Hard" } }, { 1, -1, -1, 2, 3, -1 } },
    { { { "Rage" }, { "Hard" }, { "Die" }, { "Hard" }, { "Road" }, { "Rage" } },
        { 0, 1, 3 }, { { "Road" }, { "Rage" }, { "Hard" } }, { -1, 1, 2, 2, 3, -1 } },
};

void JoypadsTest::init()
{
    takeOverInput();
}

const char *JoypadsTest::name() const
{
    return "joypads";
}

const char *JoypadsTest::displayName() const
{
    return "joypads";
}

auto JoypadsTest::getCases() -> CaseList
{
    return {
        { "test axis interval reduction", "axis-intervals", bind(&JoypadsTest::setupAxisIntervalReductionTest),
            bind(&JoypadsTest::axisIntervalReductionTest), std::size(kIntervalTestData), false },
        { "test duplicate joypads naming", "duplicate-joypad-names", bind(&JoypadsTest::setupNameSuffixAssignmentTest),
            bind(&JoypadsTest::nameSuffixAssignmentTest), std::size(kNamingTestData), false },
    };
}

void JoypadsTest::setupAxisIntervalReductionTest()
{
    constexpr const char kSectionName[] = "Freeez - V.I.P.S";
    constexpr const char kKey[] = "axes";

    m_config.clear();
    m_ini.Reset();

    auto axes = kIntervalTestData[m_currentDataIndex].input;
    m_ini.SetValue(kSectionName, kKey, axes);

    LogSilencer logSilencer;
    m_config.loadFromIni(m_ini, kSectionName);
}

void JoypadsTest::axisIntervalReductionTest()
{
    auto actual = m_config.getAxisIntervals(0);
    assertEqual(kIntervalTestData[m_currentDataIndex].expected, *actual);
}

void JoypadsTest::setupNameSuffixAssignmentTest()
{
    const auto& joypads = kNamingTestData[m_currentDataIndex].initial;
    setJoypads(joypads);
    processControlEvents();
}

void JoypadsTest::nameSuffixAssignmentTest()
{
    const auto& data = kNamingTestData[m_currentDataIndex];

    for (auto it = data.toDisconnect.rbegin(); it != data.toDisconnect.rend(); ++it)
        disconnectJoypad(*it);

    for (const auto& joypad : data.toConnect)
        connectJoypad(joypad);

    processControlEvents();

    assertEqual(getNumJoypads(), data.expectedSuffixes.size() + 1);
    for (size_t i = 0; i < data.expectedSuffixes.size(); i++)
        assertEqual(data.expectedSuffixes[i], extractNameSuffix(joypadName(i)));
}

int JoypadsTest::extractNameSuffix(const char *name)
{
    auto rParen = strrchr(name, ')');
    if (!rParen || rParen[1] != '\0' || rParen == name || rParen[-1] == '(')
        return -1;

    int suffix = 0;
    int tens = 1;
    auto p = rParen - 1;

    while (p >= name && isDigit(*p)) {
        suffix += tens * (*p-- - '0');
        tens *= 10;
    }

    if (p < name || *p != '(')
        return -1;

    return suffix;
}
