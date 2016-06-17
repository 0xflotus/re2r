// This file is part of the 're2r' package for R.
// Copyright (C) 2016, Qin Wenfeng
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.

// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.

// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
// BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// [[Rcpp::depends(RcppParallel)]]
#include <RcppParallel.h>
using namespace RcppParallel;

#include "../inst/include/re2r.h"

#include <cstddef>

#include <sstream>
#include <memory>

#define RE2R_STATIC_SIZE 10

template <typename T>
inline string NumberToString ( T Number )
{
    ostringstream ss;
    ss << ".";
    ss << Number;
    return ss.str();
}

inline void bump_count(size_t& coli,size_t& rowi, size_t cols){
    coli++;
    if (coli== cols){
        coli = 0;
        rowi++;
    }
}

inline void set_colnames(SEXP res, SEXP names){

    Rf_setAttrib(res, R_DimNamesSymbol, names);
}

SEXP toprotect_optstring_to_charmat(const optstring& res){

    Shield<SEXP>  resv(Rf_allocMatrix(STRSXP, res.size(), 1));
    SEXP dims = Rf_getAttrib(resv, R_DimSymbol);
    SEXP new_dimnames = Shield<SEXP>((Rf_allocVector(VECSXP, Rf_length(dims))));
    SET_VECTOR_ELT(new_dimnames, 1, CharacterVector::create(".match"));
    set_colnames(resv, new_dimnames );


    SEXP x = resv;

    R_xlen_t index = 0;

    for(auto dd : res){
        if (bool(dd)) {
            SET_STRING_ELT(x, index, Rf_mkCharLenCE(dd.value().c_str(),  strlen(dd.value().c_str()) , CE_UTF8));
        } else{
            SET_STRING_ELT(x, index, NA_STRING);
        }
        index++;
    }

    return resv;
}

SEXP toprotect_optstring_to_list_charmat(const optstring& optinner,  size_t cols, SEXP groups_name){

    auto rows = optinner.size() / cols;
    Shield<SEXP> res( Rf_allocMatrix(STRSXP,rows, cols));
    SEXP x = res;

    size_t rowi = 0;
    size_t coli = 0;
    for(auto dd : optinner){
        if (bool(dd)) {
            SET_STRING_ELT(x, rowi+coli*rows, Rf_mkCharLenCE(dd.value().c_str(),  strlen(dd.value().c_str()) , CE_UTF8));
        } else{
            SET_STRING_ELT(x, rowi+coli*rows, NA_STRING);
        }
        bump_count(coli, rowi, cols);
    }

    set_colnames(res, groups_name);

    return res;
}


void bump_listi(size_t cnt, List::iterator& listi, const optstring& optinner, size_t cols, SEXP groups_name){
    if(cnt == 0){ // no one match, all NA return
        *listi = R_NilValue;
    } else { // generate CharacterMatrix

        *listi = Shield<SEXP>(toprotect_optstring_to_list_charmat(optinner, cols, groups_name));
    }
    listi+=1; //bump times_n !n
}

SEXP toprotect_vec_optstring_to_charmat(const vector<optstring>& res, int cap_nums){
    auto rows = res.size();
    Shield<SEXP> resv( Rf_allocMatrix(STRSXP, rows, cap_nums));
    SEXP x = resv;

    auto rowi = 0;
    auto coli = 0;
    for(auto ind : res){
        for (const tr2::optional<string>& dd : ind){
            if (bool(dd)) {
                SET_STRING_ELT(x, rowi+coli*rows, Rf_mkCharLenCE(dd.value().c_str(),  strlen(dd.value().c_str()) , CE_UTF8));
            } else{
                SET_STRING_ELT(x, rowi+coli*rows, NA_STRING);
            }
        }
        rowi +=1;
        coli = 0;
    }
    return resv;
}

vector<string> get_groups_name(RE2* pattern, int cap_nums){
    auto groups_name = pattern->CapturingGroupNames();

    vector<int> alls;
    alls.reserve(cap_nums);
    int cnt = 1;
    while(cnt <= cap_nums){
        alls.push_back(cnt);
        cnt+=1;
    }

    vector<int> nums;
    nums.reserve(cap_nums);

    vector<string> cap_names;
    cap_names.reserve(cap_nums);

    for(auto it = groups_name.begin(); it != groups_name.end(); ++it) {
        nums.push_back(it->first);
    }

    vector<int> diff_nums(alls.size()+nums.size());

    auto diff_res = set_difference(alls.begin(),
                                   alls.end(),
                                   nums.begin(),
                                   nums.end(),
                                   diff_nums.begin());
    diff_nums.resize(diff_res-diff_nums.begin());

    for(auto ind : diff_nums) {
        groups_name.insert(make_pair(ind, NumberToString(ind)));
    }

    vector<string> res;
    res.reserve(res.size());
    for(auto it = groups_name.begin(); it!= groups_name.end(); it++) {
        res.push_back(it->second);
    }

    return res;
}

void fill_list_res(int cap_nums,
                  StringPiece* piece,
                  optstring& res, size_t cnt){
    auto all_na = true;

    // don't get all na
    if(cnt > 1){
        for(auto it = 0; it != cap_nums; ++it) {
            if((piece[it]).data() != NULL){
                all_na = false;
                break;
            }
        }
        if (all_na) return;
    }

    for(auto it = 0; it != cap_nums; ++it) {
        if((piece[it]).data() != NULL){
            res.push_back(tr2::make_optional(piece[it].as_string())) ;
        } else{
            res.push_back(tr2::nullopt);
        }
    }
}

optstring fill_opt_res(int cap_nums, StringPiece* piece, bool matched){
    optstring res(cap_nums);
    if(matched){
        auto it = res.begin();
        for(auto i = 0; i != cap_nums; ++i) {
            if((piece[i]).data() != NULL){
                *it = tr2::make_optional(piece[i].as_string());
            } else{
                *it = tr2::nullopt;
            }
            it++;
        }
    }else{
        for(auto it = res.begin(); it != res.end(); ++it) {
            *it =tr2::nullopt ;
        }
    }
    return res;
}



void fill_res(int cap_nums,
              StringPiece* piece,
              SEXP x, size_t& rowi, size_t& coli, size_t rows, size_t cols, bool matched){

    if(matched){
        for(auto it = 0; it != cap_nums; ++it) {
            if((piece[it]).data() != NULL){
                SET_STRING_ELT(x, rowi+coli*rows, Rf_mkCharLenCE(piece[it].as_string().c_str(),  strlen(piece[it].as_string().c_str()) , CE_UTF8));
            } else{
                SET_STRING_ELT(x, rowi+coli*rows, NA_STRING);
            }

            bump_count(coli,rowi, cols);
        }
    }else{
        for(size_t it = 0; it != cols; ++it) {
            SET_STRING_ELT(x, rowi+coli*rows, NA_STRING);
            bump_count(coli,rowi, cols);
        }
    }
}

RE2::Anchor get_anchor_type(size_t anchor){
    if (anchor == 0) {
        return RE2::UNANCHORED;
    } else if (anchor == 1) {
        return RE2::ANCHOR_START;
    } else {
        return RE2::ANCHOR_BOTH;
    }
}

// begin real work


SEXP cpp_detect(CharacterVector& input,
                RE2* pattern,
                RE2::Anchor anchor_type){

    SEXP inputx = input;
    LogicalVector res(input.size());
    auto resi  = res.begin();
    for(auto it = 0; it != input.size(); it++, resi ++){
        auto rstr = STRING_ELT(inputx, it);
        if (rstr == NA_STRING){
            *resi = NA_LOGICAL;
            continue;
        }
        auto r_char = R_CHAR(rstr);
        *resi = pattern->Match( r_char ,0, strlen(r_char),
                                anchor_type, nullptr, 0);
    }
    return wrap(res);
}

struct BoolP : public Worker
{
    vector<tr2::optional<string>>& input;
    RVector<int> output;
    RE2& tt;
    const RE2::Anchor anchor_type;

    BoolP (vector<tr2::optional<string>>&  input_,RVector<int> output_, RE2& tt_,const RE2::Anchor&  anchor_type_)
        : input(input_), output(output_), tt(tt_),anchor_type(anchor_type_){}

    void operator()(std::size_t begin, std::size_t end) {
        std::transform(input.begin() + begin,
                       input.begin() + end,
                       output.begin() + begin,
                       [this](tr2::optional<string>& x)->int{
                           if (!bool(x)){
                                return NA_LOGICAL;
                           }
                           return tt.Match(x.value(), 0, (int) x.value().length(),
                                                anchor_type, nullptr, 0);
                       });
    }
};


SEXP cpp_detect_parallel(CharacterVector& input,
                RE2* pattern,
                RE2::Anchor anchor_type,
                size_t grain_size){
    LogicalVector reso(input.size());
    RVector<int> res(reso);
    auto inputv = as_vec_opt_string(input);
    BoolP pobj(inputv, res, *pattern, anchor_type);
    parallelFor(0, input.size(), pobj, grain_size);
    return wrap(reso);
}


struct NoCaptureP : public Worker
{
    vector<tr2::optional<string>>& input;
    optstring& output;
    RE2& tt;
    const RE2::Anchor anchor_type;

    NoCaptureP (vector<tr2::optional<string>>&  input_, optstring& output_, RE2& tt_, const RE2::Anchor&  anchor_type_)
        : input(input_), output(output_), tt(tt_), anchor_type(anchor_type_){}

    void operator()(std::size_t begin, std::size_t end) {
        std::transform(input.begin() + begin,
                       input.begin() + end,
                       output.begin() + begin,
                       [this](tr2::optional<string>& x) -> tr2::optional<string>{
                           if (!bool(x)){
                               return tr2::nullopt;
                           }
                           if (tt.Match(x.value(), 0, (int) x.value().length(),
                                             anchor_type, nullptr, 0)){
                               return tr2::make_optional(x.value());
                           } else {
                               return tr2::nullopt;
                           };
                       });
    }
};

SEXP cpp_match_nocapture(CharacterVector& input,
                         RE2* pattern,
                         RE2::Anchor anchor_type){
    SEXP inputx = input;
    Shield<SEXP> ress(Rf_allocMatrix(STRSXP,input.size(),1));
    SEXP res = ress;

    for(auto it = 0; it!= input.size(); it++){
        auto rstr = STRING_ELT(inputx, it);
        if (rstr == NA_STRING){
            SET_STRING_ELT(res, it, NA_STRING);
            continue;
        }
        auto r_char = R_CHAR(rstr);
        if(pattern->Match(r_char,0, strlen(r_char),
                          anchor_type, nullptr, 0)){
            SET_STRING_ELT(res, it, STRING_ELT(inputx, it));
        } else {
            SET_STRING_ELT(res, it, NA_STRING);
        }

    }
    SEXP dims = Rf_getAttrib(res, R_DimSymbol);
    Shield<SEXP> new_dimnames((Rf_allocVector(VECSXP, Rf_length(dims))));
    SET_VECTOR_ELT(new_dimnames, 1, CharacterVector::create(".match"));
    set_colnames(res, new_dimnames );

    return res;
}

SEXP cpp_match_nocapture_parallel(CharacterVector& input,
                         RE2* pattern,
                         RE2::Anchor anchor_type,
                         size_t grain_size){
    optstring res(input.size());
    auto inputv = as_vec_opt_string(input);

    NoCaptureP pobj(inputv, res, *pattern, anchor_type);
    parallelFor(0, inputv.size(), pobj, grain_size);

    return toprotect_optstring_to_charmat(res);
}

# define CHECK_NA_STRING                                       \
auto rstr = STRING_ELT(inputx, it);                            \
if (rstr == NA_STRING){                                        \
    SET_STRING_ELT(res, it, NA_STRING);                        \
    continue;                                                  \
}                                                              \
auto r_char = R_CHAR(rstr);                                    \
                                                               \
for(int pn = 0; pn!=cap_nums; pn++) piece_ptr[pn].clear(); \

SEXP cpp_match_not_all(CharacterVector& input,
                       RE2* pattern,
                       RE2::Anchor anchor_type,
                       StringPiece* piece_ptr,
                       RE2::Arg** args_ptr,
                       vector<string>& groups_name,
                       int cap_nums){
    const auto cols = groups_name.size();
    const auto rows = input.size();
    size_t rowi = 0;
    size_t coli = 0;

    SEXP inputx = input;
    Shield<SEXP> ress(Rf_allocMatrix(STRSXP, input.size(), groups_name.size())); // will be constructed as Matrix
    SEXP res = ress;
    switch(anchor_type){
    case RE2::UNANCHORED:

        for(auto it = 0; it!= input.size(); it++){
            CHECK_NA_STRING

            for(int pn = 0; pn!=cap_nums; pn++) piece_ptr[pn].clear();

            fill_res(cap_nums,
                     piece_ptr, res, rowi, coli, rows, cols,
                     RE2::PartialMatchN(r_char, *pattern, args_ptr, cap_nums));
        }
        break;
    case RE2::ANCHOR_START:

        for(auto it = 0; it!= input.size(); it++){
            CHECK_NA_STRING

            for(int pn = 0; pn!=cap_nums; pn++) piece_ptr[pn].clear();
            StringPiece tmpstring(r_char);
            fill_res(cap_nums,
                     piece_ptr, res, rowi, coli, rows, cols,
                     RE2::ConsumeN(&tmpstring, *pattern, args_ptr, cap_nums));
        }
        break;
    default:

        for(auto it = 0; it!= input.size(); it++){
            CHECK_NA_STRING
            fill_res(cap_nums,
                     piece_ptr, res, rowi, coli, rows, cols,
                     RE2::FullMatchN(r_char, *pattern, args_ptr, cap_nums));
        }
        break;
    }

    // generate CharacterMatrix
    SEXP dims = Rf_getAttrib(res, R_DimSymbol);
    Shield<SEXP> new_dimnames((Rf_allocVector(VECSXP, Rf_length(dims))));
    SET_VECTOR_ELT(new_dimnames, 1, Shield<SEXP>(toprotect_vec_string_sexp(groups_name)));
    set_colnames(res, new_dimnames );
    return res;
}


#define INIT_ARGS_PTR                                              \
auto cap_nums = tt.NumberOfCapturingGroups();                 \
auto argv =  unique_ptr<RE2::Arg[]>(new RE2::Arg[cap_nums]);       \
auto args =  unique_ptr<RE2::Arg*[]>(new RE2::Arg*[cap_nums]);     \
auto piece = unique_ptr<StringPiece[]>(new StringPiece[cap_nums]); \
auto piece_ptr = piece.get();                                      \
auto args_ptr = args.get();                                        \
auto argv_ptr = argv.get();                                        \
                                                                   \
for (int nn = 0; nn != cap_nums; nn++){                            \
    args_ptr[nn] = &argv_ptr[nn];                                  \
    argv_ptr[nn] = &piece_ptr[nn];                                 \
}                                                                  \

#define UNVALUE_BLOCK                                                                              \
std::transform(input.begin() + begin,                                                              \
               input.begin() + end,                                                                \
               output.begin() + begin,                                                             \
               [this,cap_nums,piece_ptr,args_ptr](tr2::optional<string>& x) -> optstring{\
                   if (!bool(x)){                                                                  \
                       return fill_opt_res(cap_nums, piece_ptr, false) ;                           \
                   }                                                                               \
                   for(int pn = 0; pn!=cap_nums; pn++) piece_ptr[pn].clear();                      \

struct UnValue : public Worker{
    vector<tr2::optional<string>>& input;
    vector<optstring>& output;
    RE2& tt;
    const RE2::Anchor& anchor_type;

    UnValue(vector<tr2::optional<string>>&  input_, vector<optstring>& output_, RE2& tt_, const RE2::Anchor& anchor_type_)
        : input(input_), output(output_), tt(tt_), anchor_type(anchor_type_){}

    void operator()(std::size_t begin, std::size_t end) {
        INIT_ARGS_PTR

            if (anchor_type == RE2::ANCHOR_BOTH){
                UNVALUE_BLOCK
                return fill_opt_res(cap_nums,
                                    piece_ptr,
                                    tt.FullMatchN(x.value(), tt, args_ptr, cap_nums));

            });
    } else if (anchor_type == RE2::ANCHOR_START){
        UNVALUE_BLOCK
        StringPiece tmpstring(x.value());
        return fill_opt_res(cap_nums,
                            piece_ptr,
                            tt.ConsumeN(&tmpstring, tt, args_ptr, cap_nums));

    });
} else { // RE2::UNANCHORED
    UNVALUE_BLOCK
    return fill_opt_res(cap_nums,
                        piece_ptr,
                        tt.PartialMatchN(x.value(), tt, args_ptr, cap_nums));
});
    }
    }
    };


SEXP cpp_match_not_all_parallel(CharacterVector& input,
                       RE2* pattern,
                       RE2::Anchor anchor_type,
                       vector<string>& groups_name,
                       int cap_nums,
                       size_t grain_size){
    vector<optstring> output(input.size());
    auto inputv = as_vec_opt_string(input);

    UnValue pobj(inputv, output, *pattern, anchor_type);
    parallelFor(0, input.size(), pobj, grain_size);
    Shield<SEXP> res(toprotect_vec_optstring_to_charmat(output,cap_nums));

    // generate CharacterMatrix
    SEXP dims = Rf_getAttrib(res, R_DimSymbol);
    Shield<SEXP> new_dimnames((Rf_allocVector(VECSXP, Rf_length(dims))));
    SET_VECTOR_ELT(new_dimnames, 1, Shield<SEXP>(toprotect_vec_string_sexp(groups_name)));
    set_colnames(res, new_dimnames );
    return res;
}

#define INIT_LISTI_CHECKED                                                             \
optstring optinner;                                                                    \
auto rstr = STRING_ELT(inputx, it);                                                    \
if (rstr == NA_STRING){                                                                \
    bump_listi(0, listi, optinner, groups_name.size(), new_dimnames);                  \
    continue;                                                                          \
}                                                                                      \
auto r_char = R_CHAR(rstr);                                                            \
StringPiece todo_str(r_char);                                                          \
StringPiece tmp_piece = StringPiece(todo_str.data(), todo_str.length());               \
for(int pn = 0; pn!=cap_nums; pn++) piece_ptr[pn].clear();                             \
size_t cnt = 0;                                                                        \


#define CHECK_RESULT                                             \
for(int pn = 0; pn!=cap_nums; pn++) piece_ptr[pn].clear();       \
if(todo_str.length() == 0) break;                                \
                                                                 \
if((todo_str.data() == tmp_piece.data()) &&                      \
   (todo_str.length() == tmp_piece.length()) &&                  \
   (todo_str.length() !=0) ){                                    \
    todo_str.remove_prefix(1);                                   \
}                                                                \
                                                                 \
tmp_piece = StringPiece(todo_str.data(), todo_str.length());     \

SEXP cpp_match_all(CharacterVector& input,
                       RE2* pattern,
                       RE2::Anchor anchor_type,
                       StringPiece* piece_ptr,
                       RE2::Arg** args_ptr,
                       vector<string>& groups_name,
                       int cap_nums){
    List listres(input.size());
    SEXP inputx = input;
    auto listi = listres.begin();
    // for each input string, get a !n label.
    Shield<SEXP>  new_dimnames((Rf_allocVector(VECSXP, 2)));
    SET_VECTOR_ELT(new_dimnames, 1, Shield<SEXP>(toprotect_vec_string_sexp(groups_name)));

    if (anchor_type == RE2::UNANCHORED){
        for(auto it = 0; it!= input.size(); it++){

            INIT_LISTI_CHECKED
                while (RE2::FindAndConsumeN(&todo_str, *pattern, args_ptr, cap_nums)) {
                    cnt+=1;
                    fill_list_res(cap_nums, piece_ptr, optinner, cnt);

                    // Note that if the
                    // regular expression matches an empty string, input will advance
                    // by 0 bytes.  If the regular expression being used might match
                    // an empty string, the loop body must check for this case and either
                    // advance the string or break out of the loop.
                    //
                    CHECK_RESULT

                        // try next place
                }   // while
                bump_listi(cnt, listi, optinner, groups_name.size(), new_dimnames);
        }
    }
    else{
        for(auto it = 0; it!= input.size(); it++){

            INIT_LISTI_CHECKED

                while (RE2::ConsumeN(&todo_str, *pattern, args_ptr, cap_nums)) {
                    cnt+=1;
                    fill_list_res(cap_nums, piece_ptr, optinner, cnt);

                    CHECK_RESULT

                        // advanced try next place
                }   // else while
                bump_listi(cnt, listi, optinner, groups_name.size(), new_dimnames);
        }
    } // end else generate CharacterMatrix
    return listres;
}


#define INIT_LISTI                                                               \
StringPiece todo_str(ind.value());                                               \
StringPiece tmp_piece = StringPiece(todo_str.data(), todo_str.length());         \
for(int pn = 0; pn!=cap_nums; pn++) piece_ptr[pn].clear();                       \
size_t cnt = 0;                                                                  \
optstring optinner;                                                              \

struct MatValue : public Worker{
    vector<tr2::optional<string>>& input;
    vector<tr2::optional<optstring>>& output;
    RE2& tt;
    const RE2::Anchor& anchor_type;

    MatValue(vector<tr2::optional<string>>&  input_, vector<tr2::optional<optstring>>& output_, RE2& tt_, const RE2::Anchor& anchor_type_)
        : input(input_), output(output_),tt(tt_), anchor_type(anchor_type_){}

    void operator()(std::size_t begin, std::size_t end) {
        INIT_ARGS_PTR
            if (anchor_type != RE2::UNANCHORED){
                std::transform(input.begin() + begin,
                               input.begin() + end,
                               output.begin() + begin,
                               [this,cap_nums,piece_ptr,args_ptr](tr2::optional<string>& ind) -> tr2::optional<optstring>{
                                   if (!bool(ind)){
                                       return tr2::nullopt;
                                   }
                                   INIT_LISTI

                                   while (RE2::ConsumeN(&todo_str,tt, args_ptr, cap_nums)) {
                                       cnt+=1;
                                       fill_list_res(cap_nums, piece_ptr, optinner, cnt);

                                       CHECK_RESULT
                                           // advanced try next place
                                   }   // else while
                                   if (cnt == 0){
                                       return tr2::nullopt;
                                   } else {
                                       return tr2::make_optional(optinner);
                                   }
                               });
            } else{
                std::transform(input.begin() + begin,
                               input.begin() + end,
                               output.begin() + begin,
                               [this,cap_nums,piece_ptr,args_ptr](tr2::optional<string>& ind) -> tr2::optional<optstring>{
                                   if (!bool(ind)){
                                       return tr2::nullopt;
                                   }
                                   INIT_LISTI

                                   while (RE2::FindAndConsumeN(&todo_str, tt, args_ptr, cap_nums)) {
                                       cnt+=1;
                                       fill_list_res(cap_nums, piece_ptr, optinner, cnt);

                                       CHECK_RESULT
                                           // advanced try next place
                                   }   // else while
                                   if (cnt == 0){
                                       return tr2::nullopt;
                                   } else {
                                       return tr2::make_optional(optinner);
                                   }
                               });
            }
    }
};

SEXP cpp_match_all_parallel(CharacterVector& input,
                       RE2* pattern,
                       RE2::Anchor anchor_type,
                       vector<string>& groups_name,
                       int cap_nums,
                       size_t grain_size){

    List listres(input.size());

    vector<tr2::optional<optstring>> res(input.size());
    auto inputv = as_vec_opt_string(input);

    MatValue pobj(inputv, res, *pattern, anchor_type);
    parallelFor(0, input.size(), pobj,grain_size);

    Shield<SEXP>  new_dimnames((Rf_allocVector(VECSXP, 2)));
    SET_VECTOR_ELT(new_dimnames, 1, Shield<SEXP>(toprotect_vec_string_sexp(groups_name)));

    // fill in result
    auto resi = res.begin();
    for (auto it = listres.begin(); it != listres.end(); it++){
        if(!bool(*resi)){ // no one match, NULL
            *it = R_NilValue;
        } else {
            *it = Shield<SEXP>(toprotect_optstring_to_list_charmat(resi->value(), groups_name.size(), new_dimnames));
        }
        resi+=1;
    }
    return listres;
}

// [[Rcpp::export]]
SEXP cpp_match(CharacterVector input,
               XPtr<RE2>& ptr,
               bool value,
               size_t anchor,
               bool all,
               bool parallel,
               size_t grain_size){
    RE2::Anchor anchor_type = get_anchor_type(anchor);

    RE2* pattern = ptr;

    if (value == false){

        if (!parallel || input.size() < grain_size){
            return cpp_detect(input, pattern, anchor_type);
        } else {
            return cpp_detect_parallel(input, pattern, anchor_type,grain_size);
        }

    } else{
        auto cap_nums = pattern->NumberOfCapturingGroups();

        if ( cap_nums == 0){
            if (!parallel || input.size() < grain_size){
                return cpp_match_nocapture(input, pattern, anchor_type);
                // no capture group return
            } else {
                return cpp_match_nocapture_parallel(input, pattern, anchor_type, grain_size);
            }
        }

        // at least one capture group, return a matrix
        // set up the args and stringpiece
        vector<string>  g_numbers_names = get_groups_name(pattern, cap_nums);
        vector<string>  groups_name;

        // each string get at least one group of result
        groups_name.reserve(g_numbers_names.size());

        for(auto it = g_numbers_names.begin(); it!= g_numbers_names.end(); it++) {
            groups_name.push_back(*it);
        }

        if (parallel && input.size() > grain_size){
            if (all){
                return cpp_match_all_parallel(input,pattern,
                                              anchor_type,
                                              groups_name,cap_nums, grain_size);
            } else {
                return cpp_match_not_all_parallel(input,pattern,
                                                  anchor_type,
                                                  groups_name,cap_nums, grain_size);
            }

        } else{
            // not parallel, value, all
            // static when the number of capture group is smaller than 10
            RE2::Arg* args_static[RE2R_STATIC_SIZE];
            RE2::Arg  argv_static[RE2R_STATIC_SIZE];
            StringPiece piece_static[RE2R_STATIC_SIZE];

            // dynamic when the number of capture group is bigger than 10
            // We use exception, it will be better to use unique_ptr instead of raw ptr.
            unique_ptr<RE2::Arg[]> argv;
            unique_ptr<RE2::Arg*[]> args;
            unique_ptr<StringPiece[]> piece;
            // pointer to used
            RE2::Arg** args_ptr;
            StringPiece* piece_ptr;
            RE2::Arg* argv_ptr;

            // when we have a small number of capture groups,
            // we do not need to used heap
            if(cap_nums <= RE2R_STATIC_SIZE){ // RE2R_STATIC_SIZE = 10
                args_ptr = args_static;
                argv_ptr = argv_static;
                piece_ptr = piece_static;
            } else{
                argv =  unique_ptr<RE2::Arg[]>(new RE2::Arg[cap_nums]);
                args =  unique_ptr<RE2::Arg*[]>(new RE2::Arg*[cap_nums]);
                piece = unique_ptr<StringPiece[]>(new StringPiece[cap_nums]);
                piece_ptr = piece.get();
                args_ptr = args.get();
                argv_ptr = argv.get();
            };

            // It we do not manually delete this, it will be safe:
            // args_ptr， argv_ptr, piece_ptr
            // below we can only use these three ptr
            //
            for (int nn = 0; nn != cap_nums; nn++){
                args_ptr[nn] = &argv_ptr[nn];
                argv_ptr[nn] = &piece_ptr[nn];
            }

            // do the work

            if (all == false) {
                    return cpp_match_not_all(input,pattern,anchor_type, piece_ptr,
                                             args_ptr,groups_name,cap_nums);
            } else { // all == true
                    return cpp_match_all(input,pattern,anchor_type, piece_ptr,
                                         args_ptr,groups_name,cap_nums);
            } // all == true
        }
        // unique_ptr go out of scrope
    }

    throw ErrorInternal("unreachable cpp_match");
}

