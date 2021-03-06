## This file is part of the 're2r' package for R.
## Copyright (C) 2016, Qin Wenfeng
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## 1. Redistributions of source code must retain the above copyright notice,
## this list of conditions and the following disclaimer.
##
## 2. Redistributions in binary form must reproduce the above copyright notice,
## this list of conditions and the following disclaimer in the documentation
## and/or other materials provided with the distribution.
##
## 3. Neither the name of the copyright holder nor the names of its
## contributors may be used to endorse or promote products derived from
## this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
## "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
## BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
## FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
## HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
## SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
## PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
## OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
## WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
## OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
## EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#' Locate the position of patterns in a string.
#'
#' Locate the position of patterns in a string.If the match is of length 0, (e.g. from a special match like $) end will be one character less than start. Vectorised over string and pattern.
#'
#' @param pattern a character vector or pre-compiled regular expressions
#' @param string a character vector
#' @param parallel use multithread
#' @param grain_size a minimum chunk size for tuning the behavior of parallel algorithms
#' @param ... further arguments passed to \code{\link{re2}}
#' @examples
#' re2_locate("yabba dabba doo", "d")
#'
#' fruit <- c("apple", "banana", "pear", "pineapple")
#' re2_locate(fruit, "$")
#' re2_locate(fruit, "a")
#' re2_locate(fruit, "e")
#' re2_locate(fruit, c("a", "b", "p", "p"))
#'
#' re2_locate_all(fruit, "a")
#' re2_locate_all(fruit, "e")
#' re2_locate_all(fruit, c("a", "b", "p", "p"))
#'
#' # Find location of every character
#' re2_locate_all(fruit, "\\P{M}")
#'
#' @return For \code{\link{re2_locate}}, an integer matrix. First column gives start postion of match, and second column gives end position. For \code{\link{re2_locate_all}} a list of integer matrices.
#'
#' @export
re2_locate = function(string, pattern,  parallel = FALSE, grain_size= 100000, ...) {

    if (is.character(pattern) || mode(pattern) == "logical") {
        pattern = re2(pattern, ...)
    }
    cpp_locate(stri_enc_toutf8(string), pattern, FALSE, parallel, grain_size)
}

#' @rdname re2_locate
#' @export
re2_locate_all = function(string, pattern,  parallel = FALSE, grain_size= 100000, ...) {

    if (is.character(pattern) || mode(pattern) == "logical") {
        pattern = re2(pattern, ...)
    }
    cpp_locate(stri_enc_toutf8(string), pattern, TRUE, parallel, grain_size)
}
