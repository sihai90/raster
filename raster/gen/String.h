/*
 * Copyright 2017 Facebook, Inc.
 * Copyright (C) 2017, Yeolar
 */

#pragma once
#define RDD_GEN_STRING_H

#include "raster/gen/Base.h"
#include "raster/io/IOBuf.h"
#include "raster/util/Range.h"

namespace rdd {
namespace gen {

namespace detail {
class StringResplitter;

template<class Delimiter>
class SplitStringSource;

template<class Delimiter, class Output>
class Unsplit;

template<class Delimiter, class OutputBuffer>
class UnsplitBuffer;

template<class TargetContainer,
         class Delimiter,
         class... Targets>
class SplitTo;

}  // namespace detail

/**
 * Split the output from a generator into StringPiece "lines" delimited by
 * the given delimiter.  Delimters are NOT included in the output.
 *
 * resplit() behaves as if the input strings were concatenated into one long
 * string and then split.
 *
 * Equivalently, you can use StreamSplitter outside of a rdd::gen setting.
 */
// make this a template so we don't require StringResplitter to be complete
// until use
template <class S=detail::StringResplitter>
S resplit(char delimiter) {
  return S(delimiter);
}

template <class S = detail::SplitStringSource<char>>
S split(const StringPiece source, char delimiter) {
  return S(source, delimiter);
}

template <class S = detail::SplitStringSource<StringPiece>>
S split(StringPiece source, StringPiece delimiter) {
  return S(source, delimiter);
}

/**
 * EOL terms ("\r", "\n", or "\r\n").
 */
class MixedNewlines {};

/**
 * Split by EOL ("\r", "\n", or "\r\n").
 * @see split().
 */
template <class S = detail::SplitStringSource<MixedNewlines>>
S lines(StringPiece source) {
  return S(source, MixedNewlines{});
}

/*
 * Joins a sequence of tokens into a string, with the chosen delimiter.
 *
 * E.G.
 *   std::string result = split("a,b,c", ",") | unsplit(",");
 *   assert(result == "a,b,c");
 *
 *   std::string result = split("a,b,c", ",") | unsplit<std::string>(" ");
 *   assert(result == "a b c");
 */


// NOTE: The template arguments are reversed to allow the user to cleanly
// specify the output type while still inferring the type of the delimiter.
template<class Output = std::string,
         class Delimiter,
         class Unsplit = detail::Unsplit<Delimiter, Output>>
Unsplit unsplit(const Delimiter& delimiter) {
  return Unsplit(delimiter);
}

template<class Output = std::string,
         class Unsplit = detail::Unsplit<std::string, Output>>
Unsplit unsplit(const char* delimiter) {
  return Unsplit(delimiter);
}

/*
 * Joins a sequence of tokens into a string, appending them to the output
 * buffer.  If the output buffer is empty, an initial delimiter will not be
 * inserted at the start.
 *
 * E.G.
 *   std::string buffer;
 *   split("a,b,c", ",") | unsplit(",", &buffer);
 *   assert(buffer == "a,b,c");
 *
 *   std::string anotherBuffer("initial");
 *   split("a,b,c", ",") | unsplit(",", &anotherbuffer);
 *   assert(anotherBuffer == "initial,a,b,c");
 */
template<class Delimiter,
         class OutputBuffer,
         class UnsplitBuffer = detail::UnsplitBuffer<Delimiter, OutputBuffer>>
UnsplitBuffer unsplit(Delimiter delimiter, OutputBuffer* outputBuffer) {
  return UnsplitBuffer(delimiter, outputBuffer);
}

template<class OutputBuffer,
         class UnsplitBuffer = detail::UnsplitBuffer<std::string, OutputBuffer>>
UnsplitBuffer unsplit(const char* delimiter, OutputBuffer* outputBuffer) {
  return UnsplitBuffer(delimiter, outputBuffer);
}


template<class... Targets>
detail::Map<detail::SplitTo<std::tuple<Targets...>, char, Targets...>>
eachToTuple(char delim) {
  return detail::Map<
    detail::SplitTo<std::tuple<Targets...>, char, Targets...>>(
    detail::SplitTo<std::tuple<Targets...>, char, Targets...>(delim));
}

template<class... Targets>
detail::Map<detail::SplitTo<std::tuple<Targets...>, std::string, Targets...>>
eachToTuple(StringPiece delim) {
  return detail::Map<
    detail::SplitTo<std::tuple<Targets...>, std::string, Targets...>>(
    detail::SplitTo<std::tuple<Targets...>, std::string, Targets...>(delim));
}

template<class First, class Second>
detail::Map<detail::SplitTo<std::pair<First, Second>, char, First, Second>>
eachToPair(char delim) {
  return detail::Map<
    detail::SplitTo<std::pair<First, Second>, char, First, Second>>(
    detail::SplitTo<std::pair<First, Second>, char, First, Second>(delim));
}

template<class First, class Second>
detail::Map<detail::SplitTo<std::pair<First, Second>, std::string, First, Second>>
eachToPair(StringPiece delim) {
  return detail::Map<
    detail::SplitTo<std::pair<First, Second>, std::string, First, Second>>(
    detail::SplitTo<std::pair<First, Second>, std::string, First, Second>(
      to<std::string>(delim)));
}

/**
 * Outputs exactly the same bytes as the input stream, in different chunks.
 * A chunk boundary occurs after each delimiter, or, if maxLength is
 * non-zero, after maxLength bytes, whichever comes first.  Your callback
 * can return false to stop consuming the stream at any time.
 *
 * The splitter buffers the last incomplete chunk, so you must call flush()
 * to consume the piece of the stream after the final delimiter.  This piece
 * may be empty.  After a flush(), the splitter can be re-used for a new
 * stream.
 *
 * operator() and flush() return false iff your callback returns false. The
 * internal buffer is not flushed, so reusing such a splitter will have
 * indeterminate results.  Same goes if your callback throws.  Feel free to
 * fix these corner cases if needed.
 *
 * Tips:
 *  - Create via streamSplitter() to take advantage of template deduction.
 *  - If your callback needs an end-of-stream signal, test for "no
 *    trailing delimiter **and** shorter than maxLength".
 *  - You can fine-tune the initial capacity of the internal IOBuf.
 */
template <class Callback>
class StreamSplitter {

 public:
  StreamSplitter(char delimiter,
                 Callback&& pieceCb,
                 uint64_t maxLength = 0,
                 uint64_t initialCapacity = 0)
      : buffer_(IOBuf::CREATE, initialCapacity),
        delimiter_(delimiter),
        maxLength_(maxLength),
        pieceCb_(std::move(pieceCb)) {}

  /**
   * Consume any incomplete last line (may be empty). Do this before
   * destroying the StreamSplitter, or you will fail to consume part of the
   * input.
   *
   * After flush() you may proceed to consume the next stream via ().
   *
   * Returns false if the callback wants no more data, true otherwise.
   * A return value of false means that this splitter must no longer be used.
   */
  bool flush();

  /**
   * Consume another piece of the input stream.
   *
   * Returns false only if your callback refuses to consume more data by
   * returning false (true otherwise).  A return value of false means that
   * this splitter must no longer be used.
   */
  bool operator()(StringPiece in);

 private:
  // Holds the current "incomplete" chunk so that chunks can span calls to ()
  IOBuf buffer_;
  char delimiter_;
  uint64_t maxLength_;  // The callback never gets more chars than this
  Callback pieceCb_;
};

template <class Callback>  // Helper to enable template deduction
StreamSplitter<Callback> streamSplitter(char delimiter,
                                        Callback&& pieceCb,
                                        uint64_t capacity = 0) {
  return StreamSplitter<Callback>(delimiter, std::move(pieceCb), capacity);
}

}  // namespace gen
}  // namespace rdd

#include "raster/gen/String-inl.h"

