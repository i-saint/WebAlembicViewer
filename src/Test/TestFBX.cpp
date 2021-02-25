#include "pch.h"
#include "Test.h"
#include "SmallFBX/SmallFBX.h"

testCase(fbxRead)
{
    std::string path;
    test::GetArg("path", path);
    if (path.empty())
        return;

    sfbx::Document doc;
    doc.read(path);
}
