#include "readPE.h"
#include "global.h"

LPVOID* pFileBuffer = NULL;
DWORD readPeFile(IN PTCHAR lpszFile, OUT LPVOID* pFileBuffer) {
  FILE* file = NULL;
  DWORD fileSize = 0;
  _tfopen_s(&file, lpszFile, TEXT("rb"));
  if (!file) {
#ifdef DEV
    _tprintf(TEXT("cannot open file"));
#endif  // DEV
    return NULL;
  }
  // get size of file
  fseek(file, 0, SEEK_END);
  fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);
  *pFileBuffer = malloc(fileSize);
  if (!pFileBuffer) {
#ifdef DEV
    _tprintf(TEXT("malloc failed"));
#endif  // DEV
    fclose(file);
    return NULL;
  }
  size_t n = fread(*pFileBuffer, fileSize, 1, file);
  if (!n) {
#ifdef DEV
    _tprintf(TEXT("read data failed"));
#endif  // DEV
    free(*pFileBuffer);
    fclose(file);
    return NULL;
  }
  fclose(file);
  return n;
}