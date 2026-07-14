#include <cstdlib>
#include <filesystem>
#include <string>

#include <gtest/gtest.h>

#include "../tangle/tangle.h"
#include "file_comparator.h"

// *.web files from https://ctan.org/tex-archive/systems/knuth/dist
// *.ch files from https://github.com/classic-tools/TeX-FPC
// Built tangle.exe with FPC from tangle.p from https://github.com/classic-tools/TeX-FPC

struct TangleTestCase
{
    std::string           test_name;  // Name of the tool (tangle, weave, tex,...)
    std::filesystem::path webPath;
};

class TangleValidation : public ::testing::TestWithParam<TangleTestCase>
{
  protected:
    std::filesystem::path webPath;
    std::filesystem::path chPath;
    std::filesystem::path expectedPascal;
    std::filesystem::path expectedPool;
    std::filesystem::path generatedPascal;
    std::filesystem::path generatedPool;

    void
    SetUp () override
    {
        auto data_dir   = std::filesystem::path {DATA_DIR};
        auto output_dir = std::filesystem::path {OUTPUT_DIR};
        auto gen_dir    = output_dir / "generated";

        std::filesystem::create_directories (gen_dir);

        const auto &test_case = GetParam ();

        webPath               = data_dir / (test_case.test_name + ".web");
        chPath                = data_dir / (test_case.test_name + ".ch");
        expectedPascal        = data_dir / (test_case.test_name + ".p");
        expectedPool          = data_dir / (test_case.test_name + ".pool");
        generatedPascal       = gen_dir / (test_case.test_name + ".p");
        generatedPool         = gen_dir / (test_case.test_name + ".pool");
    }

    void
    compare_and_print_error (
        const std::filesystem::path &expected,
        const std::filesystem::path &generated)
    {
        const auto &test_case = GetParam ();
        auto        diff      = compare_files (expected, generated);

        EXPECT_TRUE (diff.identical)
            << "Mismatch in test case [" << test_case.test_name 
            << "] at byte position: "   << diff.byte_position
            << "\n  line number:                 " << diff.line_number
            << "\n  character position on line:  " << diff.char_on_line
            << "\n  expected character (ASCII):  " << static_cast<int> (diff.expected_char)
            << " ('" << diff.expected_char
            << "')\n  generated character (ASCII): " << static_cast<int> (diff.actual_char)
            << " ('"  << diff.actual_char << "')\n";
    }
};

TEST_P (TangleValidation, ExecutableMatchesOriginal)
{
    const auto &test_case = GetParam ();

    int         exitCode  = tangle_exceptions_handled (webPath, chPath, generatedPascal, generatedPool);
    EXPECT_EQ (exitCode, 0) << "TANGLE returned a non-zero exit code!";

    compare_and_print_error (expectedPascal, generatedPascal);
    compare_and_print_error (expectedPool, generatedPool);
}

INSTANTIATE_TEST_SUITE_P (
    BulkValidation,
    TangleValidation,
    ::testing::Values (
        TangleTestCase {"dvitype"},
        TangleTestCase {"gftopk"},
        TangleTestCase {"gftype"},
        TangleTestCase {"mf"},
        TangleTestCase {"pltotf"},
        TangleTestCase {"tangle"},
        TangleTestCase {"tex"},
        TangleTestCase {"tftopl"},
        TangleTestCase {"weave"}),
    [] (const ::testing::TestParamInfo<TangleTestCase> &info) { return info.param.test_name; });
