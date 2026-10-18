#include "NaturalSort.h"
#include <windows.h>
#include <shlwapi.h>
bool NaturalSort::less(const QString &a,const QString &b) {
    const int order=StrCmpLogicalW(reinterpret_cast<LPCWSTR>(a.utf16()),reinterpret_cast<LPCWSTR>(b.utf16()));
    return order==0?a<b:order<0;
}
