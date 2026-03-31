/*
  Copyright (C) 2002-2026 CERN for the benefit of the ATLAS collaboration
*/
// check-hist-count: sum all UHI histogram storage/values under a path.
// Usage: check-hist-count FILE.h5 [PATH]
// PATH defaults to "/".  Prints the total sum; exits 1 if sum is zero.

#include <H5Cpp.h>

#include <cstdint>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

static bool is_uhi_hist(const H5::Group& g)
{
    if (!g.attrExists("uhi_schema")) return false;
    H5::Attribute attr = g.openAttribute("uhi_schema");
    int64_t val = 0;
    attr.read(H5::PredType::NATIVE_INT64, &val);
    return val == 1;
}

static double sum_values(const H5::Group& g)
{
    H5::DataSet ds = g.openDataSet("storage/values");
    H5::DataSpace sp = ds.getSpace();
    const hsize_t n =
        static_cast<hsize_t>(sp.getSimpleExtentNpoints());
    std::vector<double> buf(n);
    ds.read(buf.data(), H5::PredType::NATIVE_DOUBLE);
    return std::accumulate(buf.begin(), buf.end(), 0.0);
}

// Recursively sum all UHI histogram storage/values under g.
static double visit(const H5::Group& g)
{
    if (is_uhi_hist(g)) return sum_values(g);
    double total = 0.0;
    for (hsize_t i = 0; i < g.getNumObjs(); ++i) {
        const std::string name = g.getObjnameByIdx(i);
        if (g.childObjType(name) != H5O_TYPE_GROUP) continue;
        total += visit(g.openGroup(name));
    }
    return total;
}

int main(int argc, char* argv[])
{
    if (argc < 2 || argc > 3) {
        std::cerr << "usage: check-hist-count FILE.h5 [PATH]\n";
        return 2;
    }
    H5::Exception::dontPrint();
    const H5::H5File file(argv[1], H5F_ACC_RDONLY);
    const std::string path = (argc == 3) ? argv[2] : "/";
    const double total = visit(file.openGroup(path));
    std::cout << total << "\n";
    return total == 0.0 ? 1 : 0;
}
