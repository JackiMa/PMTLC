#ifndef SIPIN_PDE_TABLE_HH
#define SIPIN_PDE_TABLE_HH

#include <string>
#include <vector>

// A lightweight (λ,θ) → p_det table loader for SiPIN boundary modeling.
// Expected CSV format (long form, one sample per row):
//   wavelength_nm, theta_deg, p_det
// Lines starting with '#' are ignored. A header line is allowed.
//
// Interpolation: bilinear on the regular grid built from unique λ and θ.
// Out-of-range queries are clamped to the table boundaries.
class SipinPDETable {
public:
  bool LoadFromCsv(const std::string& path, std::string* errMsg = nullptr);
  bool IsLoaded() const { return loaded_; }
  const std::string& Path() const { return path_; }

  // wavelength_nm: in nm
  // theta_deg: incidence angle in degrees, 0 = normal incidence
  double GetPdet(double wavelength_nm, double theta_deg) const;

private:
  static bool ParseRow3(const std::string& line, double& wl_nm, double& theta_deg, double& pdet);
  static double Clamp(double x, double lo, double hi);
  static int FindLowerIndex(const std::vector<double>& grid, double x);
  double GetAt(size_t iTheta, size_t iWl) const;

  bool loaded_ = false;
  std::string path_;
  std::vector<double> wl_nm_;
  std::vector<double> theta_deg_;
  // row-major: p_[iTheta * wl_nm_.size() + iWl]
  std::vector<double> p_;
};

#endif




