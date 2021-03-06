#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/SourceMgr.h"
#include <iostream>

// yeah it's a hot one

using namespace llvm;

int main() {
  auto FileName = __FILE__;
  ErrorOr<std::unique_ptr<MemoryBuffer>> MemBufferOrErr =
      MemoryBuffer::getFile(FileName);
  if (!MemBufferOrErr) {
    std::error_code ErrorCode = MemBufferOrErr.getError();
    std::cerr << "An error occurred when opening file \"" << FileName
              << "\": " << ErrorCode << std::endl;
    return 1;
  }

  auto ParentPath = sys::path::parent_path(FileName);
  SmallString<128> IncludeFileName = ParentPath;
  sys::path::append(IncludeFileName, "CMakeLists.txt");
  ErrorOr<std::unique_ptr<MemoryBuffer>> IncludeMemBufferOrErr =
    MemoryBuffer::getFile(IncludeFileName.str());
  if (!IncludeMemBufferOrErr) {
    std::error_code ErrorCode = MemBufferOrErr.getError();
    std::cerr << "An error occurred when opening file \""
              << IncludeFileName.str().str()
              << "\": " << ErrorCode << std::endl;
    return 1;
  }

  SourceMgr SM;
  SM.AddNewSourceBuffer(std::move(IncludeMemBufferOrErr.get()),
                        /*IncludeLoc*/ SMLoc());
  const MemoryBuffer *IncludeMemBuffer = SM.getMemoryBuffer(SM.getMainFileID());
  SMLoc IncludeLoc = SMLoc::getFromPointer(IncludeMemBuffer->getBufferStart());
  SM.PrintMessage(
      IncludeLoc, SourceMgr::DiagKind::DK_Note,
      "This is the SMLoc the rest of the diagnostics will refer to as the "
      "include location.");

  SM.AddNewSourceBuffer(std::move(MemBufferOrErr.get()),
                        IncludeLoc);
  const MemoryBuffer *MemBuffer = SM.getMemoryBuffer(2);

  SMLoc FirstLoc = SMLoc::getFromPointer(MemBuffer->getBufferStart());
  SM.PrintMessage(FirstLoc, SourceMgr::DiagKind::DK_Remark,
                  "This is the first SMLoc in this file.");

  const char *CurrentChar = MemBuffer->getBufferStart();
  while (*CurrentChar != '\n')
    ++CurrentChar;
  SMLoc EndOfFirstLineLoc = SMLoc::getFromPointer(--CurrentChar);
  SM.PrintMessage(EndOfFirstLineLoc, SourceMgr::DiagKind::DK_Remark,
                  "This is the first line in this file.",
                  SMRange(FirstLoc, EndOfFirstLineLoc));

  ++CurrentChar;
  SMLoc NextLineLoc = SMLoc::getFromPointer(++CurrentChar);
  SM.PrintMessage(NextLineLoc, SourceMgr::DiagKind::DK_Warning,
                  "This SMRange spans two lines, but SourceMgr will only "
                  "display the second line.",
                  SMRange(FirstLoc, NextLineLoc));

  while (*CurrentChar != '<')
    ++CurrentChar;
  SMLoc StartOfIOStreamLoc = SMLoc::getFromPointer(++CurrentChar);
  while (*CurrentChar != '>')
    ++CurrentChar;
  SMLoc EndOfIOStreamLoc = SMLoc::getFromPointer(CurrentChar);
  SM.PrintMessage(
      StartOfIOStreamLoc, SourceMgr::DiagKind::DK_Remark,
      "Wouldn't you rather use <OHIOstream>?", None,
      SMFixIt(SMRange(StartOfIOStreamLoc, EndOfIOStreamLoc), "OHIOstream"));

  return 0;
}
