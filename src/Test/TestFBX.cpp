#include "pch.h"
#include "Test.h"
#include "SmallFBX/SmallFBX.h"

testCase(fbxRead)
{
    std::string path;
    test::GetArg("path", path);
    if (path.empty())
        return;

    sfbx::DocumentPtr doc = sfbx::MakeDocument();
    doc->read(path);

    auto obj = doc->findNode("Objects");
}
