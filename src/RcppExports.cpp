// This file was generated by Rcpp::compileAttributes
// Generator token: 10BE3573-1514-4C36-9D1C-5A225CD40393

#include "../inst/include/re2r.h"
#include <Rcpp.h>

using namespace Rcpp;

// re2_cpp_compile
XPtr<RE2> re2_cpp_compile(const char * pattern, bool log_errors_value, bool utf_8_value, bool posix_syntax_value, bool case_sensitive_value, bool dot_nl_value, bool literal_value, bool longest_match_value, bool never_nl_value, bool never_capture_value, bool one_line_value, bool perl_classes_value, bool word_boundary_value, int64_t max_mem_value);
RcppExport SEXP re2r_re2_cpp_compile(SEXP patternSEXP, SEXP log_errors_valueSEXP, SEXP utf_8_valueSEXP, SEXP posix_syntax_valueSEXP, SEXP case_sensitive_valueSEXP, SEXP dot_nl_valueSEXP, SEXP literal_valueSEXP, SEXP longest_match_valueSEXP, SEXP never_nl_valueSEXP, SEXP never_capture_valueSEXP, SEXP one_line_valueSEXP, SEXP perl_classes_valueSEXP, SEXP word_boundary_valueSEXP, SEXP max_mem_valueSEXP) {
BEGIN_RCPP
    Rcpp::RObject __result;
    Rcpp::RNGScope __rngScope;
    Rcpp::traits::input_parameter< const char * >::type pattern(patternSEXP);
    Rcpp::traits::input_parameter< bool >::type log_errors_value(log_errors_valueSEXP);
    Rcpp::traits::input_parameter< bool >::type utf_8_value(utf_8_valueSEXP);
    Rcpp::traits::input_parameter< bool >::type posix_syntax_value(posix_syntax_valueSEXP);
    Rcpp::traits::input_parameter< bool >::type case_sensitive_value(case_sensitive_valueSEXP);
    Rcpp::traits::input_parameter< bool >::type dot_nl_value(dot_nl_valueSEXP);
    Rcpp::traits::input_parameter< bool >::type literal_value(literal_valueSEXP);
    Rcpp::traits::input_parameter< bool >::type longest_match_value(longest_match_valueSEXP);
    Rcpp::traits::input_parameter< bool >::type never_nl_value(never_nl_valueSEXP);
    Rcpp::traits::input_parameter< bool >::type never_capture_value(never_capture_valueSEXP);
    Rcpp::traits::input_parameter< bool >::type one_line_value(one_line_valueSEXP);
    Rcpp::traits::input_parameter< bool >::type perl_classes_value(perl_classes_valueSEXP);
    Rcpp::traits::input_parameter< bool >::type word_boundary_value(word_boundary_valueSEXP);
    Rcpp::traits::input_parameter< int64_t >::type max_mem_value(max_mem_valueSEXP);
    __result = Rcpp::wrap(re2_cpp_compile(pattern, log_errors_value, utf_8_value, posix_syntax_value, case_sensitive_value, dot_nl_value, literal_value, longest_match_value, never_nl_value, never_capture_value, one_line_value, perl_classes_value, word_boundary_value, max_mem_value));
    return __result;
END_RCPP
}
