#include "SipinPDETable.hh"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

namespace {
static inline std::string Trim(const std::string& s) {
  size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) b++;
  size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) e--;
  return s.substr(b, e - b);
}

static inline bool StartsWith(const std::string& s, const char* pfx) {
  for (size_t i = 0; pfx[i]; ++i) {
    if (i >= s.size() || s[i] != pfx[i]) return false;
  }
  return true;
}
} // namespace

bool SipinPDETable::ParseRow3(const std::string& line, double& wl_nm, double& theta_deg, double& pdet) {
  // Split by comma/space/tab/semicolon
  std::string tmp = line;
  for (char& c : tmp) {
    if (c == ',' || c == ';' || c == '\t') c = ' ';
  }
  std::istringstream iss(tmp);
  if (!(iss >> wl_nm >> theta_deg >> pdet)) return false;
  return true;
}

double SipinPDETable::Clamp(double x, double lo, double hi) {
  if (x < lo) return lo;
  if (x > hi) return hi;
  return x;
}

int SipinPDETable::FindLowerIndex(const std::vector<double>& grid, double x) {
  // returns i such that grid[i] <= x <= grid[i+1], clamped to [0, n-2]
  if (grid.size() < 2) return 0;
  if (x <= grid.front()) return 0;
  if (x >= grid.back()) return static_cast<int>(grid.size() - 2);
  auto it = std::upper_bound(grid.begin(), grid.end(), x);
  int idx = static_cast<int>(std::distance(grid.begin(), it)) - 1;
  if (idx < 0) idx = 0;
  if (idx > static_cast<int>(grid.size() - 2)) idx = static_cast<int>(grid.size() - 2);
  return idx;
}

double SipinPDETable::GetAt(size_t iTheta, size_t iWl) const {
  const size_t nW = wl_nm_.size();
  return p_[iTheta * nW + iWl];
}

bool SipinPDETable::LoadFromCsv(const std::string& path, std::string* errMsg) {
  loaded_ = false;
  path_.clear();
  wl_nm_.clear();
  theta_deg_.clear();
  p_.clear();

  std::ifstream fin(path);
  if (!fin) {
    if (errMsg) *errMsg = "Cannot open file: " + path;
    return false;
  }

  std::set<double> wlSet;
  std::set<double> thSet;
  struct Key {
    double wl;
    double th;
    bool operator<(const Key& o) const {
      if (th < o.th) return true;
      if (th > o.th) return false;
      return wl < o.wl;
    }
  };
  std::map<Key, double> samples;

  std::string line;
  bool sawAny = false;
  while (std::getline(fin, line)) {
    line = Trim(line);
    if (line.empty()) continue;
    if (StartsWith(line, "#")) continue;

    double wl = 0, th = 0, p = 0;
    if (!ParseRow3(line, wl, th, p)) {
      // likely header; skip
      continue;
    }
    sawAny = true;
    wlSet.insert(wl);
    thSet.insert(th);
    samples[{wl, th}] = p;
  }

  if (!sawAny || wlSet.size() < 2 || thSet.size() < 2) {
    if (errMsg) *errMsg = "CSV does not contain enough samples (need >=2 wavelengths and >=2 angles).";
    return false;
  }

  wl_nm_.assign(wlSet.begin(), wlSet.end());
  theta_deg_.assign(thSet.begin(), thSet.end());
  const size_t nW = wl_nm_.size();
  const size_t nT = theta_deg_.size();
  p_.assign(nT * nW, 0.0);

  // Fill grid; if missing samples exist, keep 0 and report.
  size_t missing = 0;
  for (size_t iT = 0; iT < nT; ++iT) {
    for (size_t iW = 0; iW < nW; ++iW) {
      const Key k{wl_nm_[iW], theta_deg_[iT]};
      auto it = samples.find(k);
      if (it == samples.end()) {
        missing++;
        p_[iT * nW + iW] = 0.0;
      } else {
        p_[iT * nW + iW] = it->second;
      }
    }
  }
  if (missing > 0 && errMsg) {
    std::ostringstream oss;
    oss << "CSV grid has missing samples: " << missing << " (filled with 0).";
    *errMsg = oss.str();
  }

  loaded_ = true;
  path_ = path;
  return true;
}

double SipinPDETable::GetPdet(double wavelength_nm, double theta_deg) const {
  if (!loaded_ || wl_nm_.empty() || theta_deg_.empty()) return 0.0;

  // clamp to table bounds
  const double wl = Clamp(wavelength_nm, wl_nm_.front(), wl_nm_.back());
  const double th = Clamp(theta_deg, theta_deg_.front(), theta_deg_.back());

  const int iW = FindLowerIndex(wl_nm_, wl);
  const int iT = FindLowerIndex(theta_deg_, th);

  const double wl0 = wl_nm_[iW];
  const double wl1 = wl_nm_[iW + 1];
  const double th0 = theta_deg_[iT];
  const double th1 = theta_deg_[iT + 1];

  const double tx = (wl1 > wl0) ? (wl - wl0) / (wl1 - wl0) : 0.0;
  const double ty = (th1 > th0) ? (th - th0) / (th1 - th0) : 0.0;

  const double p00 = GetAt(iT, iW);
  const double p10 = GetAt(iT, iW + 1);
  const double p01 = GetAt(iT + 1, iW);
  const double p11 = GetAt(iT + 1, iW + 1);

  const double p0 = p00 + tx * (p10 - p00);
  const double p1 = p01 + tx * (p11 - p01);
  const double p = p0 + ty * (p1 - p0);

  // physical clamp
  return Clamp(p, 0.0, 1.0);
}


