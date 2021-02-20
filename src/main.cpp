#include "pch.h"
#include "WebAlembicViewer.h"

#pragma comment(lib, "Alembic.lib")
#pragma comment(lib, "Half-2_5.lib")
#pragma comment(lib, "Iex-2_5.lib")
#pragma comment(lib, "IexMath-2_5.lib")
#pragma comment(lib, "Imath-2_5.lib")


int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("no input files\n");
        return 0;
    }

    auto scene = wabc::CreateScene();
    if (scene->load(argv[1])) {
        auto time_range = scene->getTimeRange();
        printf("%s\n", argv[1]);
        printf("time range: %lf %lf\n", std::get<0>(time_range), std::get<1>(time_range));

        scene->seek(0.0);
        auto mesh = scene->getMesh();
        printf("vertex count: %d\n", (int)mesh->getPoints().size());
    }
}
