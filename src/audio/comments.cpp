#include "comments.h"
#include "util.h"
#include "file.h"
#include "audio.h"
#include "chants.h"
#include "SoundSample.h"
#include <dirent.h>

static std::array<SoundSample, 76 + 52> m_originalCommentarySamples;

static bool m_hasAudioDir;

static SoundSample m_endGameCrowdSample;

static Mix_Chunk *m_lastPlayedComment;
static size_t m_lastPlayedCommentHash;
static int m_lastPlayedCategory;

static int m_commentaryChannel = -1;

//
// sample loading
//

struct SampleTable {
    const char *dir;
    std::vector<SoundSample> samples;
    int lastPlayedIndex = -1;

    SampleTable(const char *dir) : dir(dir) {}
};

// these must match exactly with directories in the sample tables below
enum CommentarySampleTableIndex {
    kCorner, kDirtyTackle, kEndGameRout, kEndGameSensational, kEndGameSoClose, kFoul, kGoal, kGoodPass, kGoodTackle,
    kHeader, kHitBar, kHitPost, kInjury, kKeeperClaimed, kKeeperSaved, kNearMiss, kOwnGoal, kPenalty, kPenaltyMissed,
    kPenaltySaved, kPenaltyGoal, kRedCard, kSubstitution, kChangeTactics, kThrowIn, kYellowCard,
    kNumSampleTables,
};

// all the comments heard in the game
static std::array<SampleTable, kNumSampleTables> m_sampleTables = {{
    { "corner" },
    { "dirty_tackle" },
    { "end_game_rout" },
    { "end_game_sensational" },
    { "end_game_so_close" },
    { "free_kick" },
    { "goal" },
    { "good_play" },
    { "good_tackle" },
    { "header" },
    { "hit_bar" },
    { "hit_post" },
    { "injury" },
    { "keeper_claimed" },
    { "keeper_saved" },
    { "near_miss" },
    { "own_goal" },
    { "penalty" },
    { "penalty_missed" },
    { "penalty_saved" },
    { "penalty_scored" },
    { "red_card" },
    { "substitution" },
    { "tactic_changed" },
    { "throw_in" },
    { "yellow_card" },
}};

void initCommentsBeforeTheGame()
{
    m_commentaryChannel = -1;

    m_lastPlayedCategory = -1;
    m_lastPlayedComment = nullptr;

    m_endGameCrowdSample.free();
}

bool commenteryOnChannelFinished(int channel)
{
    if (channel == m_commentaryChannel) {
        logDebug("Commentary finished playing on channel %d", channel);
        m_commentaryChannel = -1;
        return true;
    }

    return false;
}

static int parseSampleChanceMultiplier(const char *str, size_t len)
{
    assert(str);
    assert(len == strlen(str));

    int frequency = 1;

    if (len >= 4) {
        auto end = str + len - 1;

        for (auto p = end; p >= str; p--) {
            if (*p == '.') {
                end = p - 1;
                break;
            }
        }

        if (end - str >= 4 && isdigit(*end)) {
            int value = 0;
            int power = 1;
            auto digit = end;

            while (digit >= str && isdigit(*digit)) {
                value += power * (*digit - '0');
                power *= 10;
                digit--;
            }

            if (digit >= str + 2 && *digit == 'x' && (digit[-1] == '_' || digit[-1] == '-'))
                frequency = value;
        }
    }

    return frequency;
}

static size_t getBasenameLength(const char *filename, size_t len)
{
    assert(filename);

    for (int i = static_cast<int>(len) - 1; i >= 0; i--)
        if (filename[i] == '.')
            return i ? i - 1 : 0;

    return len;
}

static void loadCustomCommentary()
{
    assert(m_sampleTables.size() == kNumSampleTables);
    assert(!strcmp(m_sampleTables[kEndGameSoClose].dir, "end_game_so_close"));
    assert(!strcmp(m_sampleTables[kYellowCard].dir, "yellow_card"));

    const std::string audioPath = joinPaths("audio", "commentary") + getDirSeparator();

    for (int i = 0; i < kNumSampleTables; i++) {
        auto& table = m_sampleTables[i];
        table.lastPlayedIndex = -1;

        if (!table.samples.empty())
            continue;

        const auto& dirPath = audioPath + table.dir + getDirSeparator();
        const auto& dirFullPath = pathInRootDir((audioPath + table.dir).c_str());
        auto dir = opendir(dirFullPath.c_str());

        if (dir) {
            for (dirent *entry; entry = readdir(dir); ) {
                if (entry->d_namlen < 4)
                    continue;

                auto samplePath = dirPath + entry->d_name;
                int chance = parseSampleChanceMultiplier(entry->d_name, entry->d_namlen);

                SoundSample sample(samplePath.c_str(), chance);
                if (sample.hasData()) {
                    table.samples.push_back(sample);
                    logInfo("`%s' loaded OK, chance: %d", samplePath.c_str(), chance);
                } else {
                    logWarn("Failed to load sample `%s'", samplePath.c_str());
                }
            }

            closedir(dir);
        }
    }
}

static void mapOriginalSamples()
{
    struct SampleMapping {
        CommentarySampleTableIndex tableIndex;
        std::pair<int, int> sampleIndexRange;
    } static const kSampleMapping[] = {
        // preloaded comments
        { kGoal, { 0, 6 } },
        { kKeeperSaved, { 6, 13 } },
        { kOwnGoal, { 13, 19 } },
        { kNearMiss, { 19, 21 } },
        { kNearMiss, { 28, 30 } },
        { kKeeperClaimed, { 21, 28 } },
        { kHitPost, { 28, 36 } },
        { kHitBar, { 35, 37 } },
        { kGoodTackle, { 38, 42 } },
        { kGoodPass, { 42, 46 } },
        { kFoul, { 46, 49 } },
        { kDirtyTackle, { 46, 53 } },
        { kPenalty, { 53, 57 } },
        { kPenaltySaved, { 57, 60 } },
        { kPenaltySaved, { 6, 8 } },
        { kPenaltySaved, { 11, 13 } },
        { kPenaltySaved, { 9, 10 } },
        { kPenaltyMissed, { 60, 64 } },
        { kPenaltyGoal, { 64, 68 } },
        { kHeader, { 68, 76 } },
        // on-demand comments
        { kCorner, { 76, 82 } },
        { kChangeTactics, { 82, 88 } },
        { kSubstitution, { 88, 94 } },
        { kRedCard, { 94, 106 } },
        { kEndGameSoClose, { 106, 108 } },
        { kEndGameRout, { 108, 109 } },
        { kEndGameSensational, { 109, 111 } },
        { kYellowCard, { 111, 124 } },
        { kThrowIn, { 124, 128 } },
    };

    for (const auto& mapping : kSampleMapping) {
        auto& table = m_sampleTables[mapping.tableIndex];

        if (!table.samples.empty())
            continue;

        const auto& range = mapping.sampleIndexRange;

        for (int i = range.first; i < range.second; i++) {
            const auto& sampleData = m_originalCommentarySamples[i];
            if (sampleData.hasData())
                table.samples.emplace_back(sampleData);
        }
    }

    // all the samples that have double chance of getting played
    static const std::tuple<CommentarySampleTableIndex, int, int> kDoubleChanceSamples[] = {
        { kGoal, 0, 2 },
        { kKeeperSaved, 6, 7 },
        { kOwnGoal, 15, 17 },
        { kFoul, 46, 47 },
        { kDirtyTackle, 46, 47 },
        { kCorner, 77, 79 },
        { kChangeTactics, 82, 84 },
        { kSubstitution, 88, 90 },
        { kRedCard, 94, 98 },
        { kYellowCard, 111, 114 },
    };

    for (const auto& sampleData : kDoubleChanceSamples) {
        auto& table = m_sampleTables[std::get<0>(sampleData)];
        auto start = std::get<1>(sampleData);
        auto end = std::get<2>(sampleData);

        while (start < end) {
            const auto& sampleData = m_originalCommentarySamples[start];
            if (sampleData.hasData()) {
                auto it = std::find(table.samples.begin(), table.samples.end(), sampleData);
                if (it != table.samples.end())
                    it->setChanceModifier(2);
            }
            start++;
        }
    }
}

static void loadOriginalSamples()
{
    // unique samples used in tables of on-demand comment filenames
    static const char *kOnDemandSamples[] = {
        aHardM10_v__raw, aHardM10_w__raw, aHardM10_y__raw, aHardM313_1__ra, aHardM313_2__ra, aHardM313_3__ra, aHardM10_5__raw,
        aHardM10_7__raw, aHardM10_8__raw, aHardM10_9__raw, aHardM10_a__raw, aHardM10_b__raw, aHardM233_j__ra, aHardM233_k__ra,
        aHardM233_l__ra, aHardM233_m__ra, aHardM10_3__raw, aHardM10_4__raw, aHardM196_8__ra, aHardM196_9__ra, aHardM196_a__ra,
        aHardM196_b__ra, aHardM196_c__ra, aHardM196_d__ra, aHardM196_e__ra, aHardM196_f__ra, aHardM196_g__ra, aHardM196_h__ra,
        aHardM196_i__ra, aHardM196_j__ra, aHardM406_f__ra, aHardM406_g__ra, aHardM406_h__ra, aHardM406_i__ra, aHardM406_j__ra,
        aHardM443_7__ra, aHardM443_8__ra, aHardM443_9__ra, aHardM443_a__ra, aHardM443_b__ra, aHardM443_c__ra, aHardM443_d__ra,
        aHardM443_e__ra, aHardM443_f__ra, aHardM443_g__ra, aHardM443_h__ra, aHardM443_i__ra, aHardM443_j__ra, aHardM406_3__ra,
        aHardM406_8__ra, aHardM406_7__ra, aHardM406_9__ra,
        kSentinel,
    };

    int i = 0;
    for (auto ptr : { commentaryTable, kOnDemandSamples }) {
        for (; *ptr != kSentinel; ptr++, i++) {
            auto& sampleData = m_originalCommentarySamples[i];
            if (!sampleData.hasData())
                sampleData.loadFromFile(*ptr);
        }
    }

    assert(i == m_originalCommentarySamples.size());

    mapOriginalSamples();
}

static void initGameAudio();

void SWOS::LoadCommentary()
{
    if (g_soundOff || !g_commentary)
        return;

    logInfo("Loading commentary...");

    auto audioDirPath = pathInRootDir("audio");
    auto audioDir = opendir(audioDirPath.c_str());

    m_hasAudioDir = audioDir != nullptr;

    if (m_hasAudioDir) {
        closedir(audioDir);
        loadCustomCommentary();
    } else {
        loadOriginalSamples();
    }
}

static bool commentPlaying()
{
    return m_commentaryChannel >= 0 && Mix_Playing(m_commentaryChannel);
}

static void playComment(Mix_Chunk *chunk, bool interrupt = true)
{
    assert(chunk);

    if (commentPlaying()) {
        if (!interrupt) {
            logDebug("Playing comment aborted since the previous comment is still playing");
            return;
        }

        logDebug("Interrupting previous comment");
        Mix_HaltChannel(m_commentaryChannel);
    }

    Mix_VolumeChunk(chunk, MIX_MAX_VOLUME);
    m_commentaryChannel = Mix_PlayChannel(-1, chunk, 0);

    m_lastPlayedComment = chunk;
    m_lastPlayedCommentHash = hash(chunk->abuf, chunk->alen);
}

static bool isLastPlayedComment(SoundSample& sample)
{
    assert(sample.isChunkLoaded());

    if (m_lastPlayedComment) {
        auto chunk = sample.chunk();
        return sample.hash() == m_lastPlayedCommentHash && chunk->alen == m_lastPlayedComment->alen &&
            !memcmp(chunk->abuf, m_lastPlayedComment->abuf, chunk->alen);
    }

    return false;
}

static void playComment(CommentarySampleTableIndex tableIndex, bool interrupt = true)
{
    if (g_soundOff || !g_commentary || g_muteCommentary)
        return;

    auto& table = m_sampleTables[tableIndex];
    auto& samples = table.samples;

    if (!samples.empty()) {
        int range = 0;
        for (const auto& sample : samples)
            range += sample.chanceModifier();

        int i = getRandomInRange(0, range - 1);

        size_t j = 0;
        for (; j < samples.size(); j++) {
            i -= samples[j].chanceModifier();
            if (i < 0)
                break;
        }

        assert(j < static_cast<int>(samples.size()));

        while (!samples.empty()) {
            auto chunk = samples[j].chunk();

            if (!chunk) {
                logWarn("Failed to load comment %d from category %d", j, tableIndex);
                samples.erase(samples.begin() + j);
                table.lastPlayedIndex -= table.lastPlayedIndex >= static_cast<int>(j);
                j -= j == samples.size();
                continue;
            }

            if (samples.size() > 1 && (table.lastPlayedIndex == j || isLastPlayedComment(samples[j]))) {
                j = (j + 1) % samples.size();
                continue;
            }

            if (samples.size() > 2 && table.lastPlayedIndex == j) {
                j = (j + 1) % samples.size();
                continue;
            }

            logDebug("Playing comment %d from category %d", j, tableIndex);
            playComment(chunk, interrupt);
            table.lastPlayedIndex = j;
            m_lastPlayedCategory = tableIndex;
            return;
        }
    } else {
        logWarn("Comment table %d empty", tableIndex);
    }
}

//
// SWOS sound hooks
//

// in:
//     A6 -> player's team
//
void SWOS::PlayerDoingHeader_157()
{
    auto team = A6.as<TeamGeneralInfo *>();
    assert(team);

    if (team->playerNumber)
        playComment(kHeader, false);
}

// in:
//     A6 -> player's team
//
void SWOS::PlayerTackled_59()
{
    auto team = A6.as<TeamGeneralInfo *>();
    assert(team);

    if (team->playerNumber)
        playComment(kInjury);
}

void SWOS::PlayPenaltyGoalComment()
{
    playComment(kPenaltyGoal);
    performingPenalty = 0;
}

void SWOS::PlayPenaltyMissComment()
{
    playComment(kPenaltyMissed);
    performingPenalty = 0;
}

void SWOS::PlayPenaltySavedComment()
{
    playComment(kPenaltySaved);
    performingPenalty = 0;
}

void SWOS::PlayPenaltyComment()
{
    performingPenalty = -1;
    playComment(kPenalty);
}

void SWOS::PlayFoulComment()
{
    playComment(kFoul);
}

void SWOS::PlayDangerousPlayComment()
{
    playComment(kDirtyTackle);
}

void SWOS::PlayGoodPassComment()
{
    playComment(kGoodPass, false);
}

void SWOS::PlayGoodTackleComment()
{
    playComment(kGoodTackle, false);
}

void SWOS::PlayPostHitComment()
{
    playComment(kHitPost);
}

void SWOS::PlayBarHitComment()
{
    playComment(kHitBar);
}

void SWOS::PlayKeeperClaimedComment()
{
    if (performingPenalty) {
        playComment(kPenaltySaved);
        performingPenalty = 0;
    } else {
        // don't interrupt penalty saved comments
        if (m_lastPlayedCategory != kPenaltySaved || !commentPlaying())
            playComment(kKeeperClaimed);
    }
}

void SWOS::PlayNearMissComment()
{
    if (performingPenalty || penaltiesState == -1)
        PlayPenaltyMissComment();
    else
        playComment(kNearMiss);
}

void SWOS::PlayGoalkeeperSavedComment()
{
    if (performingPenalty || penaltiesState == -1)
        PlayPenaltySavedComment();
    else
        playComment(kKeeperSaved);
}

// fix original SWOS bug where penalty flag remains set when penalty is missed, but it's not near miss
void SWOS::CheckIfBallOutOfPlay_251()
{
    performingPenalty = 0;
}

void SWOS::PlayOwnGoalComment()
{
    playComment(kOwnGoal);
}

void SWOS::PlayGoalComment()
{
    if (performingPenalty || penaltiesState == -1)
        PlayPenaltyGoalComment();
    else
        playComment(kGoal);
}

void SWOS::PlayYellowCardSample()
{
    playComment(kYellowCard);
}

void SWOS::PlayRedCardSample()
{
    playComment(kRedCard);
}

void SWOS::PlayCornerSample()
{
    playComment(kCorner);
}

void SWOS::PlayTacticsChangeSample()
{
    playComment(kChangeTactics);
}

void SWOS::PlaySubstituteSample()
{
    playComment(kSubstitution);
}

void SWOS::PlayThrowInSample()
{
    playComment(kThrowIn);
}

void SWOS::PlayEnqueuedSamples()
{
    if (playingYellowCardTimer >= 0 && !--playingYellowCardTimer) {
        PlayYellowCardSample();
        playingYellowCardTimer = -1;
    } else if (playingRedCardTimer >= 0 && !--playingRedCardTimer) {
        PlayRedCardSample();
        playingRedCardTimer = -1;
    } else if (playingGoodPassTimer >= 0 && !--playingGoodPassTimer) {
        PlayGoodPassComment();
        playingGoodPassTimer = -1;
    } else if (playingThrowInSample >= 0 && !--playingThrowInSample) {
        PlayThrowInSample();
        playingThrowInSample = -1;
    } else if (playingCornerSample >= 0 && !--playingCornerSample) {
        PlayCornerSample();
        playingCornerSample = -1;
    } else if (substituteSampleTimer >= 0 && !--substituteSampleTimer) {
        PlaySubstituteSample();
        substituteSampleTimer = -1;
    } else if (tacticsChangedSampleTimer >= 0 && !--tacticsChangedSampleTimer) {
        PlayTacticsChangeSample();
        tacticsChangedSampleTimer = -1;
    }

    // strange place to decrement this...
    if (goalCounter)
        goalCounter--;

    playCrowdChants();
}

static void loadAndPlayEndGameCrowdSample(int index)
{
    if (g_trainingGame || !g_commentary)
        return;

    assert(index >= 0 && index <= 3);

    // no need to worry about this still being played
    m_endGameCrowdSample.free();

    auto filename = endGameCrowdSamples[index];

    std::string path;
    if (m_hasAudioDir)
        path = joinPaths("audio", "fx") + getDirSeparator() + (filename + 5);   // skip "hard\" part

    m_endGameCrowdSample.loadFromFile(m_hasAudioDir ? path.c_str() : filename);

    auto chunk = m_endGameCrowdSample.chunk();
    if (chunk) {
        Mix_VolumeChunk(chunk, MIX_MAX_VOLUME);
        Mix_PlayChannel(-1, chunk, 0);
    } else {
        logWarn("Failed to load end game sample %s", filename);
    }
}

void SWOS::LoadAndPlayEndGameComment()
{
    if (g_soundOff || !g_commentary)
        return;

    int index;
    // weird way of comparing, it might be like this because of team positions?
    if (team1GoalsDigit2 == team2GoalsDigit2)
        index = 2;
    else if (team2GoalsDigit2 < team1GoalsDigit2)
        index = 0;
    else
        index = 1;

    loadAndPlayEndGameCrowdSample(index);

    int goalDiff = std::abs(statsTeam1Goals - statsTeam2Goals);

    if (goalDiff >= 3)
        PlayItsBeenACompleteRoutComment();
    else if (!goalDiff)
        PlayDrawComment();
    else if (statsTeam1Goals + statsTeam2Goals >= 4)
        PlaySensationalGameComment();
}

void SWOS::PlayDrawComment()
{
    playComment(kEndGameSoClose);
}

void SWOS::PlaySensationalGameComment()
{
    playComment(kEndGameSensational);
}

void SWOS::PlayItsBeenACompleteRoutComment()
{
    playComment(kEndGameRout);
}
