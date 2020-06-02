#pragma once

#include "BaseTest.h"

class CommentaryTest : public BaseTest
{
    void init() override;
    void finish() override;
    void defaultCaseInit() override {}
    const char *name() const override;
    const char *displayName() const override;
    CaseList getCases() override;

private:
    using EnqueuedSamplesData = std::vector<std::pair<int16_t *, void (*)()>>;

    void setupOriginalSamplesLoadingTest();
    void testOriginalSamples();
    void setupCustomSamplesLoadingTest();
    void testCustomSamples();
    void setupLoadingCustomExtensionsTest();
    void testLoadingCustomExtensions();
    void setupAudioOverridingOriginalCommentsTest();
    void testAudioOverridingOriginalComments();
    void setupHandlingBadFileTest();
    void testHandlingBadFile();
    void setupCommentInterruptionTest();
    void testCommentInterruption();
    void setupMutingCommentsTest();
    void testMutingComments();

    void testIfOriginalSamplesLoaded();
    void testOriginalEnqueuedSamples();
    void applyEnqueuedSamplesData(const EnqueuedSamplesData& data, const std::vector<int>& values);
    bool createNextPermutation(std::vector<int>& values);
    void *getPlaybackFunctionFromChunk(const Mix_Chunk *chunk);
    void testResultChants();
    void testEndGameCrowdSample();
    void testEndGameComment();

    static const CommentaryTest::EnqueuedSamplesData kEnqueuedSamplesData;
};
