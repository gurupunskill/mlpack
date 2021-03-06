/**
 * @file fastmks_main.cpp
 * @author Ryan Curtin
 *
 * Main executable for maximum inner product search.
 *
 * mlpack is free software; you may redistribute it and/or modify it under the
 * terms of the 3-clause BSD license.  You should have received a copy of the
 * 3-clause BSD license along with mlpack.  If not, see
 * http://www.opensource.org/licenses/BSD-3-Clause for more information.
 */
#include <mlpack/prereqs.hpp>
#include <mlpack/core/util/cli.hpp>
#include <mlpack/core/util/mlpack_main.hpp>

#include "fastmks.hpp"
#include "fastmks_model.hpp"

using namespace std;
using namespace mlpack;
using namespace mlpack::fastmks;
using namespace mlpack::kernel;
using namespace mlpack::tree;
using namespace mlpack::metric;
using namespace mlpack::util;

PROGRAM_INFO("FastMKS (Fast Max-Kernel Search)",
    "This program will find the k maximum kernels of a set of points, "
    "using a query set and a reference set (which can optionally be the same "
    "set). More specifically, for each point in the query set, the k points in"
    " the reference set with maximum kernel evaluations are found.  The kernel "
    "function used is specified with the " + PRINT_PARAM_STRING("kernel") +
    " parameter."
    "\n\n"
    "For example, the following command will calculate, for each point in the "
    "query set " + PRINT_DATASET("query") + ", the five points in the "
    "reference set " + PRINT_DATASET("reference") + " with maximum kernel "
    "evaluation using the linear kernel.  The kernel evaluations may be saved "
    "with the  " + PRINT_DATASET("kernels") + " output parameter and the "
    "indices may be saved with the " + PRINT_DATASET("indices") + " output "
    "parameter."
    "\n\n" +
    PRINT_CALL("fastmks", "k", 5, "reference", "reference", "query", "query",
        "indices", "indices", "kernels", "kernels", "kernel", "linear") +
    "\n\n"
    "The output matrices are organized such that row i and column j in the "
    "indices matrix corresponds to the index of the point in the reference set "
    "that has j'th largest kernel evaluation with the point in the query set "
    "with index i.  Row i and column j in the kernels matrix corresponds to the"
    " kernel evaluation between those two points."
    "\n\n"
    "This program performs FastMKS using a cover tree.  The base used to build "
    "the cover tree can be specified with the " + PRINT_PARAM_STRING("base") +
    " parameter.");

// Model-building parameters.
PARAM_MATRIX_IN("reference", "The reference dataset.", "r");
PARAM_STRING_IN("kernel", "Kernel type to use: 'linear', 'polynomial', "
    "'cosine', 'gaussian', 'epanechnikov', 'triangular', 'hyptan'.", "K",
    "linear");
PARAM_DOUBLE_IN("base", "Base to use during cover tree construction.", "b",
    2.0);

// Kernel parameters.
PARAM_DOUBLE_IN("degree", "Degree of polynomial kernel.", "d", 2.0);
PARAM_DOUBLE_IN("offset", "Offset of kernel (for polynomial and hyptan "
    "kernels).", "o", 0.0);
PARAM_DOUBLE_IN("bandwidth", "Bandwidth (for Gaussian, Epanechnikov, and "
    "triangular kernels).", "w", 1.0);
PARAM_DOUBLE_IN("scale", "Scale of kernel (for hyptan kernel).", "s", 1.0);

// Load/save models.
PARAM_MODEL_IN(FastMKSModel, "input_model", "Input FastMKS model to use.", "m");
PARAM_MODEL_OUT(FastMKSModel, "output_model", "Output for FastMKS model.", "M");

// Search preferences.
PARAM_MATRIX_IN("query", "The query dataset.", "q");
PARAM_INT_IN("k", "Number of maximum kernels to find.", "k", 0);
PARAM_FLAG("naive", "If true, O(n^2) naive mode is used for computation.", "N");
PARAM_FLAG("single", "If true, single-tree search is used (as opposed to "
    "dual-tree search.", "S");

PARAM_MATRIX_OUT("kernels", "Output matrix of kernels.", "p");
PARAM_UMATRIX_OUT("indices", "Output matrix of indices.", "i");

void mlpackMain()
{
  // Validate command-line parameters.
  RequireOnlyOnePassed({ "reference", "input_model" }, true);

  ReportIgnoredParam({{ "input_model", true }}, "kernel");
  ReportIgnoredParam({{ "input_model", true }}, "bandwidth");
  ReportIgnoredParam({{ "input_model", true }}, "degree");
  ReportIgnoredParam({{ "input_model", true }}, "offset");

  ReportIgnoredParam({{ "k", false }}, "indices");
  ReportIgnoredParam({{ "k", false }}, "kernels");
  ReportIgnoredParam({{ "k", false }}, "query");

  if (CLI::HasParam("k"))
  {
    RequireAtLeastOnePassed({ "indices", "kernels" }, false,
        "no output will be saved");
  }

  // Check on kernel type.
  RequireParamInSet<string>("kernel", { "linear", "polynomial", "cosine",
      "gaussian", "triangular", "hyptan", "epanechnikov" }, true,
      "unknown kernel type");

  // Naive mode overrides single mode.
  ReportIgnoredParam({{ "naive", true }}, "single");

  FastMKSModel model;
  arma::mat referenceData;
  if (CLI::HasParam("reference"))
  {
    referenceData = std::move(CLI::GetParam<arma::mat>("reference"));

    Log::Info << "Loaded reference data (" << referenceData.n_rows << " x "
        << referenceData.n_cols << ")." << endl;

    // For cover tree construction.
    const double base = CLI::GetParam<double>("base");

    // Kernel parameters.
    const string kernelType = CLI::GetParam<string>("kernel");
    const double degree = CLI::GetParam<double>("degree");
    const double offset = CLI::GetParam<double>("offset");
    const double bandwidth = CLI::GetParam<double>("bandwidth");
    const double scale = CLI::GetParam<double>("scale");

    // Search preferences.
    const bool naive = CLI::HasParam("naive");
    const bool single = CLI::HasParam("single");

    if (kernelType == "linear")
    {
      LinearKernel lk;
      model.KernelType() = FastMKSModel::LINEAR_KERNEL;
      model.BuildModel(referenceData, lk, single, naive, base);
    }
    else if (kernelType == "polynomial")
    {
      PolynomialKernel pk(degree, offset);
      model.KernelType() = FastMKSModel::POLYNOMIAL_KERNEL;
      model.BuildModel(referenceData, pk, single, naive, base);
    }
    else if (kernelType == "cosine")
    {
      CosineDistance cd;
      model.KernelType() = FastMKSModel::COSINE_DISTANCE;
      model.BuildModel(referenceData, cd, single, naive, base);
    }
    else if (kernelType == "gaussian")
    {
      GaussianKernel gk(bandwidth);
      model.KernelType() = FastMKSModel::GAUSSIAN_KERNEL;
      model.BuildModel(referenceData, gk, single, naive, base);
    }
    else if (kernelType == "epanechnikov")
    {
      EpanechnikovKernel ek(bandwidth);
      model.KernelType() = FastMKSModel::EPANECHNIKOV_KERNEL;
      model.BuildModel(referenceData, ek, single, naive, base);
    }
    else if (kernelType == "triangular")
    {
      TriangularKernel tk(bandwidth);
      model.KernelType() = FastMKSModel::TRIANGULAR_KERNEL;
      model.BuildModel(referenceData, tk, single, naive, base);
    }
    else if (kernelType == "hyptan")
    {
      HyperbolicTangentKernel htk(scale, offset);
      model.KernelType() = FastMKSModel::HYPTAN_KERNEL;
      model.BuildModel(referenceData, htk, single, naive, base);
    }
  }
  else
  {
    // Load model from file, then do whatever is necessary.
    model = std::move(CLI::GetParam<FastMKSModel>("input_model"));
  }

  // Set search preferences.
  model.Naive() = CLI::HasParam("naive");
  model.SingleMode() = CLI::HasParam("single");

  // Should we do search?
  if (CLI::HasParam("k"))
  {
    arma::mat kernels;
    arma::Mat<size_t> indices;

    if (CLI::HasParam("query"))
    {
      const double base = CLI::GetParam<double>("base");

      arma::mat queryData = std::move(CLI::GetParam<arma::mat>("query"));

      Log::Info << "Loaded query data (" << queryData.n_rows << " x "
          << queryData.n_cols << ")." << endl;

      model.Search(queryData, (size_t) CLI::GetParam<int>("k"), indices,
          kernels, base);
    }
    else
    {
      model.Search((size_t) CLI::GetParam<int>("k"), indices, kernels);
    }

    // Save output, if we were asked to.
    if (CLI::HasParam("kernels"))
      CLI::GetParam<arma::mat>("kernels") = std::move(kernels);

    if (CLI::HasParam("indices"))
      CLI::GetParam<arma::Mat<size_t>>("indices") = std::move(indices);
  }

  // Save the model, if requested.
  if (CLI::HasParam("output_model"))
    CLI::GetParam<FastMKSModel>("output_model") = std::move(model);
}
