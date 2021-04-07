#pragma once

#include "BaseTest.h"
#include "JoypadConfig.h"

class JoypadsTest : public BaseTest
{
    void init() override;
    void finish() override {}
    void defaultCaseInit() override {}
    const char *name() const override;
    const char *displayName() const override;
    CaseList getCases() override;

private:
    void setupAxisIntervalReductionTest();
    void axisIntervalReductionTest();
    void setupNameSuffixAssignmentTest();
    void nameSuffixAssignmentTest();
    int extractNameSuffix(const char *name);

    CSimpleIni m_ini;
    JoypadConfig m_config;
};
