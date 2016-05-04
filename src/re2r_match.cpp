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

#include "../inst/include/re2r.h"

// [[Rcpp::depends(RcppParallel)]]
#include <RcppParallel.h>
using namespace RcppParallel;

#include <cstddef>

#include <sstream>
#include <memory>

#define RE2R_STATIC_SIZE 10

template <typename T>
inline string NumberToString ( T Number )
{
    ostringstream ss;
    ss << "?";
    ss << Number;
    return ss.str();
}

template <typename T>
inline string numbertostring ( T Number )
{
    ostringstream ss;
    ss << Number;
    return ss.str();
}


void bump_count(size_t& rowi,size_t& coli, size_t rows){
    rowi++;
    if (rowi== rows){
        rowi = 0;
        coli++;
    }
}

CharacterMatrix optstring_to_charmat(const optstring& res){

    CharacterMatrix resv(res.size(), 1);
    colnames(resv) = CharacterVector::create("?nocapture");
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

CharacterMatrix optstring_to_list_charmat(const optstring& optinner, const vector<string>& groups_name){
    auto rows = groups_name.size();
    CharacterMatrix res(optinner.size() / groups_name.size(), groups_name.size());

    size_t rowi = 0;
    size_t coli = 0;
    for(auto dd : optinner){
        if (bool(dd)) {
            res(coli,rowi) = dd.value();
        } else{
            res(coli,rowi) = NA_STRING;
        }
        bump_count(rowi, coli, rows);
    }

    colnames(res) = wrap(groups_name);
    return res;
}

void bump_listi(size_t cnt, List::iterator& listi, const optstring& optinner, const vector<string>& groups_name){
    if(cnt == 0){ // no one match, all NA return
        *listi = R_NilValue;
    } else { // generate CharacterMatrix

        *listi = optstring_to_list_charmat(optinner, groups_name);
    }
    listi+=1; //bump times_n !n
}

CharacterMatrix vec_optstring_to_charmat(const vector<optstring>& res, int cap_nums){

    CharacterMatrix resv(res.size(), cap_nums);
    auto rowi = 0;
    auto coli = 0;
    for(auto dd : res){
        for (auto ee : dd){
            if (bool(ee)) {
                resv(rowi, coli) = ee.value();
            } else{
                resv(rowi, coli) = NA_STRING;
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

void fill_all_res(string& times_n,
                  int cap_nums,
                  StringPiece* piece,
                  optstring& res, size_t cnt,bool matched){
    auto all_na = true;

    // don't get all na
    if(cnt > 1 && matched ==true ){
        for(auto it = 0; it != cap_nums; ++it) {
            if((piece[it]).data() != NULL){
                all_na = false;
                break;
            }
        }
        if (all_na) return;
    }

    res.push_back(tr2::make_optional(times_n));

    if(matched){
        for(auto it = 0; it != cap_nums; ++it) {
            if((piece[it]).data() != NULL){
                res.push_back(tr2::make_optional(piece[it].as_string())) ;
            } else{
                res.push_back(tr2::nullopt);
            }
        }
    }else{
        for(auto it = 0; it != cap_nums; ++it) {
            res.push_back(tr2::nullopt);
        }
    }
}

void fill_list_res(int cap_nums,
                  StringPiece* piece,
                  optstring& res, size_t cnt,bool matched){
    auto all_na = true;

    // don't get all na
    if(cnt > 1 && matched ==true ){
        for(auto it = 0; it != cap_nums; ++it) {
            if((piece[it]).data() != NULL){
                all_na = false;
                break;
            }
        }
        if (all_na) return;
    }

    if(matched){
        for(auto it = 0; it != cap_nums; ++it) {
            if((piece[it]).data() != NULL){
                res.push_back(tr2::make_optional(piece[it].as_string())) ;
            } else{
                res.push_back(tr2::nullopt);
            }
        }
    }else{
        for(auto it = 0; it != cap_nums; ++it) {
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
              CharacterMatrix& res, size_t& rowi, size_t& coli, size_t rows, bool matched){
    if(matched){
        for(auto it = 0; it != cap_nums; ++it) {
            if((piece[it]).data() != NULL){
                res(coli,rowi) = piece[it].as_string();
            } else{
                res(coli,rowi) = NA_STRING;
            }
            bump_count(rowi,coli, rows);
        }
    }else{
        for(auto it = 0; it != cap_nums; ++it) {
            res(coli,rowi) = NA_STRING;
            bump_count(rowi,coli, rows);
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

struct BoolP : public Worker
{
    const vector<string>& input;
    vector<bool>& output;
    RE2* tt;
    const RE2::Anchor anchor_type;

    BoolP (const vector<string>&  input_,vector<bool>& output_, RE2* tt_, const RE2::Anchor&  anchor_type_)
        : input(input_), output(output_), tt(tt_), anchor_type(anchor_type_){}

    void operator()(std::size_t begin, std::size_t end) {
        std::transform(input.begin() + begin,
                       input.begin() + end,
                       output.begin() + begin,
                       [this](const string& x){
                           return tt->Match(x, 0, (int) x.length(),
                                                 anchor_type, nullptr, 0);
                           });
    }
};

struct NoCaptureP : public Worker
{
    const vector<string>& input;
    optstring& output;
    RE2* tt;
    const RE2::Anchor anchor_type;

    NoCaptureP (const vector<string>&  input_, optstring& output_, RE2* tt_, const RE2::Anchor&  anchor_type_)
        : input(input_), output(output_), tt(tt_), anchor_type(anchor_type_){}

    void operator()(std::size_t begin, std::size_t end) {
        std::transform(input.begin() + begin,
                       input.begin() + end,
                       output.begin() + begin,
                       [this](const string& x) -> tr2::optional<string>{
                           if (tt->Match(x, 0, (int) x.length(),
                                            anchor_type, nullptr, 0)){
                               return tr2::make_optional(x);
                           } else {
                               return tr2::nullopt;
                           };
                       });
    }
};

#define INIT_ARGS_PTR                                          \
auto cap_nums = tt->NumberOfCapturingGroups();                 \
auto argv =  unique_ptr<RE2::Arg[]>(new RE2::Arg[cap_nums]);   \
auto args =  unique_ptr<RE2::Arg*[]>(new RE2::Arg*[cap_nums]); \
auto piece = unique_ptr<StringPiece[]>(new StringPiece[cap_nums]); \
auto piece_ptr = piece.get();                                  \
auto args_ptr = args.get();                                    \
auto argv_ptr = argv.get();                                    \
                                                               \
for (int nn = 0; nn != cap_nums; nn++){                        \
    args_ptr[nn] = &argv_ptr[nn];                              \
    argv_ptr[nn] = &piece_ptr[nn];                             \
}                                                              \


#define UNVALUE_BLOCK                                          \
    std::transform(input.begin() + begin,                      \
                   input.begin() + end,                        \
                   output.begin() + begin,                     \
    [this,cap_nums,piece_ptr,args_ptr](const string& x) -> optstring{\
        for(int pn = 0; pn!=cap_nums; pn++) piece_ptr[pn].clear(); \



struct UnValue : public Worker{
    const vector<string>& input;
    vector<optstring>& output;
    RE2* tt;
    const RE2::Anchor& anchor_type;

    UnValue(const vector<string>&  input_, vector<optstring>& output_, RE2* tt_,const RE2::Anchor& anchor_type_)
        : input(input_), output(output_), tt(tt_),anchor_type(anchor_type_){}

    void operator()(std::size_t begin, std::size_t end) {
        INIT_ARGS_PTR

        if (anchor_type == RE2::ANCHOR_BOTH){
            UNVALUE_BLOCK
            return fill_opt_res(cap_nums,
                                piece_ptr,
                                tt->FullMatchN(x, *tt, args_ptr, cap_nums));

                           });
        } else if (anchor_type == RE2::ANCHOR_START){
           UNVALUE_BLOCK
           StringPiece tmpstring(x);
           return fill_opt_res(cap_nums,
                               piece_ptr,
                               tt->ConsumeN(&tmpstring, *tt, args_ptr, cap_nums));

                           });
        } else { // RE2::UNANCHORED
            UNVALUE_BLOCK
            return fill_opt_res(cap_nums,
                                piece_ptr,
                                tt->PartialMatchN(x, *tt, args_ptr, cap_nums));
                           });
        }
    }
};


#define INIT_LISTI                                                       \
StringPiece todo_str(ind);                                               \
StringPiece tmp_piece = StringPiece(todo_str.data(), todo_str.length()); \
for(int pn = 0; pn!=cap_nums; pn++) piece_ptr[pn].clear();               \
size_t cnt = 0;                                                          \
optstring optinner;                                                      \


#define INIT_CHARM                                                       \
StringPiece todo_str(ind);                                               \
StringPiece tmp_piece = StringPiece(todo_str.data(), todo_str.length()); \
for(int pn = 0; pn!=cap_nums; pn++) piece_ptr[pn].clear();               \
size_t cnt = 0;                                                \

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


struct MatValue : public Worker{
    const vector<string>& input;
    vector<tr2::optional<optstring>>& output;
    RE2* tt;
    const RE2::Anchor& anchor_type;

    MatValue(const vector<string>&  input_, vector<tr2::optional<optstring>>& output_, RE2* tt_,const RE2::Anchor& anchor_type_)
        : input(input_), output(output_), tt(tt_),anchor_type(anchor_type_){}

    void operator()(std::size_t begin, std::size_t end) {
        INIT_ARGS_PTR
        if (anchor_type != RE2::UNANCHORED){
            std::transform(input.begin() + begin,
                           input.begin() + end,
                           output.begin() + begin,
                           [this,cap_nums,piece_ptr,args_ptr](const string& ind) -> tr2::optional<optstring>{
                                INIT_LISTI

                                while (RE2::ConsumeN(&todo_str, *tt, args_ptr, cap_nums)) {
                                    cnt+=1;
                                    fill_list_res(cap_nums, piece_ptr, optinner, cnt, true);

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
                           [this,cap_nums,piece_ptr,args_ptr](const string& ind) -> tr2::optional<optstring>{
                               INIT_LISTI

                               while (RE2::FindAndConsumeN(&todo_str, *tt, args_ptr, cap_nums)) {
                                   cnt+=1;
                                   fill_list_res(cap_nums, piece_ptr, optinner, cnt, true);

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

// [[Rcpp::export]]
SEXP cpp_match(vector<string>& input,
               XPtr<RE2>& ptr,
               bool value,
               size_t anchor,
               bool all,
               bool tolist,
               bool parallel){
    RE2::Anchor anchor_type = get_anchor_type(anchor);

    auto pattern = ptr.checked_get();

    if (value == false){
        vector<bool> res;

        if (!parallel){
            res.reserve(input.size());
            for(const string& ind : input){
                res.push_back(pattern->Match(ind,0,(int) ind.length(),
                                             anchor_type, nullptr, 0));
            }
        } else {
            res.resize(input.size());
            BoolP pobj(input, res, pattern, anchor_type);
            parallelFor(0, input.size(), pobj);
        }

        return wrap(res);
        // bool return, the fastest one
    } else{
        auto cap_nums = pattern->NumberOfCapturingGroups();

        if ( cap_nums == 0){
            if (!parallel){
                CharacterMatrix res(input.size(),1);
                auto ip = input.begin();
                for(auto it = res.begin(); it!= res.end(); it++){
                    if(pattern->Match(*ip,0,(int) ip->length(),
                                      anchor_type, nullptr, 0)){
                        *it = *ip;
                    } else {
                        *it = NA_STRING;
                    }
                    ip++;
                }
                colnames(res) = CharacterVector::create("?nocapture");
                return wrap(res);
                // no capture group return
            } else {
                optstring res(input.size());
                NoCaptureP pobj(input, res, pattern, anchor_type);
                parallelFor(0, input.size(), pobj);

                CharacterMatrix resm = optstring_to_charmat(res);

                return resm;
            }
        }

        // at least one capture group, return a data.frame

        // set up the args and stringpiece
        vector<string>  g_numbers_names = get_groups_name(pattern, cap_nums);
        vector<string>  groups_name;
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
        // each string get a group of result
        if (all == false) {

            groups_name.reserve(g_numbers_names.size());
            for(auto it = g_numbers_names.begin(); it!= g_numbers_names.end(); it++) {
                groups_name.push_back(*it);
            }
            CharacterMatrix res;
            const auto rows = groups_name.size();
            size_t rowi = 0;
            size_t coli = 0;
            if (!parallel){
                switch(anchor_type){
                case RE2::UNANCHORED:
                    res = CharacterMatrix(input.size(),groups_name.size()); // will be constructed as Matrix

                    for(const string& ind : input){
                        for(int pn = 0; pn!=cap_nums; pn++) piece_ptr[pn].clear();

                        fill_res(cap_nums,
                                 piece_ptr, res, rowi, coli, rows,
                                 RE2::PartialMatchN(ind, *pattern, args_ptr, cap_nums));
                    }
                    break;
                case RE2::ANCHOR_START:
                    res = CharacterMatrix(input.size(),groups_name.size()); // will be constructed as Matrix

                    for(const string& ind : input){
                        for(int pn = 0; pn!=cap_nums; pn++) piece_ptr[pn].clear();
                        StringPiece tmpstring(ind);
                        fill_res(cap_nums,
                                 piece_ptr, res, rowi, coli, rows,
                                 RE2::ConsumeN(&tmpstring, *pattern, args_ptr, cap_nums));
                    }
                    break;
                default:
                    res = CharacterMatrix(input.size(),groups_name.size()); // will be constructed as Matrix

                for(const string& ind : input){
                    for(int pn = 0; pn!=cap_nums; pn++) piece_ptr[pn].clear();

                    fill_res(cap_nums,
                             piece_ptr, res, rowi, coli, rows,
                             RE2::FullMatchN(ind, *pattern, args_ptr, cap_nums));
                }
                break;
                }
            } else {
                vector<optstring> output(input.size());
                UnValue pobj(input, output, pattern, anchor_type);
                parallelFor(0, input.size(), pobj);
                res = vec_optstring_to_charmat(output,cap_nums);
            }

            // generate CharacterMatrix
            colnames(res) = wrap(groups_name);
            return wrap(res);

        } else { // all == true

            if (!tolist){
                // each string get at least one group of result
                groups_name.reserve(g_numbers_names.size()+1);
                groups_name.push_back("!n");

                for(auto it = g_numbers_names.begin(); it!= g_numbers_names.end(); it++) {
                    groups_name.push_back(*it);
                }

                optstring optres;
                // for each input string, get a !n label.
                size_t times_n = 1;

                if (!parallel){
                    if (anchor_type == RE2::UNANCHORED){
                        for(const string& ind : input){
                            INIT_CHARM
                            while (RE2::FindAndConsumeN(&todo_str, *pattern, args_ptr, cap_nums)) {
                                cnt+=1;
                                string numstring = numbertostring(times_n);
                                fill_all_res(numstring, cap_nums, piece_ptr, optres, cnt, true);

                                // Note that if the
                                // regular expression matches an empty string, input will advance
                                // by 0 bytes.  If the regular expression being used might match
                                // an empty string, the loop body must check for this case and either
                                // advance the string or break out of the loop.
                                //
                                CHECK_RESULT

                                // try next place
                            }   // while

                            if(cnt == 0){ // no one match, all NA return
                                string numstring = numbertostring(times_n);
                                fill_all_res(numstring, cap_nums, piece_ptr, optres, cnt, false);
                            }
                            times_n+=1; //bump times_n !n
                        }
                        }
                    else{
                        for(const string& ind : input){
                            INIT_CHARM
                            while (RE2::ConsumeN(&todo_str, *pattern, args_ptr, cap_nums)) {
                                cnt+=1;
                                string numstring = numbertostring(times_n);
                                fill_all_res(numstring, cap_nums, piece_ptr, optres, cnt, true);

                                CHECK_RESULT

                                // advanced try next place
                            }   // else while

                            if(cnt == 0){ // no one match, all NA return
                                string numstring = numbertostring(times_n);
                                fill_all_res(numstring, cap_nums, piece_ptr, optres, cnt, false);
                            }
                            times_n+=1; //bump times_n !n
                        }
                    } // end else

                    return optstring_to_list_charmat(optres, groups_name);
                } // !tolist !parallel
                else{ // !tolist parallel
                    vector<tr2::optional<optstring>> res(input.size());
                    MatValue pobj(input, res, pattern, anchor_type);
                    parallelFor(0, input.size(), pobj);
                    size_t rows = 0;
                    for (auto it = res.begin(); it != res.end(); it++){
                        if(bool(*it)){ // no one match, all NA return
                            rows += it->value().size() / (groups_name.size()-1);
                        } else {
                            rows += 1;
                        }
                    }
                    CharacterMatrix resm(rows, groups_name.size());

                    size_t rowi = 0;
                    size_t coli = 0;

                    for(auto dd : res){

                        if (bool(dd)) {
                            for (tr2::optional<string>& inner : dd.value()){

                                if (coli == 0){
                                    resm(rowi,0) = numbertostring(times_n);
                                    coli++;
                                }

                                if(bool(inner)){
                                    resm(rowi,coli) = inner.value();
                                }else {
                                    resm(rowi,coli) = NA_STRING;
                                }
                                coli++;

                                if(coli == groups_name.size()){
                                    coli = 0;
                                    rowi++;
                                }
                            }

                        } else{
                            resm(rowi,coli) = numbertostring(times_n);
                            coli++;
                            while(coli!=groups_name.size()){
                                resm(rowi,coli) = NA_STRING;
                                coli++;
                            }
                            rowi++;
                        }
                        times_n++;
                    }

                    colnames(resm) = wrap(groups_name);
                    return resm;
                }
            } // tolist == false
            else{ // tolist == true

                // each string get at least one group of result
                groups_name.reserve(g_numbers_names.size());

                for(auto it = g_numbers_names.begin(); it!= g_numbers_names.end(); it++) {
                    groups_name.push_back(*it);
                }

                List listres(input.size());

                if (!parallel){
                    auto listi = listres.begin();
                    // for each input string, get a !n label.

                    if (anchor_type == RE2::UNANCHORED){
                        for(const string& ind : input){

                            INIT_LISTI

                            while (RE2::FindAndConsumeN(&todo_str, *pattern, args_ptr, cap_nums)) {
                                cnt+=1;
                                fill_list_res(cap_nums, piece_ptr, optinner, cnt, true);

                                CHECK_RESULT

                                    // try next place
                            }   // while
                            bump_listi(cnt, listi, optinner, groups_name);
                        }
                    }
                    else{
                        for(const string& ind : input){

                            INIT_LISTI

                            while (RE2::ConsumeN(&todo_str, *pattern, args_ptr, cap_nums)) {
                                cnt+=1;
                                fill_list_res(cap_nums, piece_ptr, optinner, cnt, true);

                                CHECK_RESULT

                                    // advanced try next place
                            }   // else while
                            bump_listi(cnt, listi, optinner, groups_name);
                        }
                    } // end else generate CharacterMatrix
                } else {
                    // parallel compute
                    vector<tr2::optional<optstring>> res(input.size());
                    MatValue pobj(input, res, pattern, anchor_type);
                    parallelFor(0, input.size(), pobj);

                    // fill in result
                    auto resi = res.begin();
                    for (auto it = listres.begin(); it != listres.end(); it++){
                        if(!bool(*resi)){ // no one match, NULL
                            *it = R_NilValue;
                        } else {
                            *it = optstring_to_list_charmat(resi->value(), groups_name);
                        }
                        resi+=1;
                    }
                }
                    return wrap(listres);

            } // tolist == true

        } // all == true

        // unique_ptr go out of scrope
    }
    throw ErrorInternal("unreachable cpp_match");
}

