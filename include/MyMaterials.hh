// Lab 27 CERN

#ifndef MyMaterials_hh
#define MyMaterials_hh

#include "G4Material.hh"
#include "G4MaterialTable.hh"
#include "G4Element.hh"
#include "G4ElementTable.hh"
#include "G4OpticalSurface.hh"


class MyMaterials
{
private:

public:
  MyMaterials();
  ~MyMaterials();

  static G4Material* Vacuum();
  static G4Material* Air();
  static G4Material* AirKiller(); // special material with same prop of air but absorbing light
  static G4Material* Water();
  static G4Material* Silicon();
  static G4Material* Aluminium();
  static G4Material* Iron();
  static G4Material* Lead();
  static G4Material* Brass();
  static G4Material* Tungsten();
  static G4Material* TungstenLight();
  static G4Material* CopperTungstenAlloy(const G4double& WFrac);
  static G4Material* Quartz();
  static G4Material* Borosilicate();
  static G4Material* OpticalGrease();
  static G4Material* LSO();
  static G4Material* LYSO(double user_lightyield,double scaleFactor,double user_birks);
  static G4Material* LuAG_Ce(); //
  static G4Material* LuAG_Pr();
  static G4Material* DSB_Ce();
  static G4Material* SiO2_Ce();
  static G4Material* BGO            (double user_lightyield,double scaleFactor,double user_birks);
  static G4Material* PWO            (double user_lightyield,double scaleFactor,double user_birks);
  static G4Material* YAG_Ce         (double user_lightyield,double scaleFactor,double user_birks);
  static G4Material* GAGG_Ce_Mg     (double user_lightyield,double scaleFactor,double user_birks);
  static G4Material* GAGG_ILM       (double user_lightyield,double scaleFactor,double user_birks);
  static G4Material* GFAG           (double user_lightyield,double scaleFactor,double user_birks);
  static G4Material* GYAGG          (double user_lightyield,double scaleFactor,double user_birks);
  static G4Material* GAGG_very_fast (double user_lightyield,double scaleFactor,double user_birks);
  static G4Material* GAGG_slow      (double user_lightyield,double scaleFactor,double user_birks);
  static G4Material* Polystyrene    (double user_lightyield,double scaleFactor,double user_birks);
  static G4Material* PLEX           (double scaleFactor);
  static G4Material* FlurPoly       (double scaleFactor);
  static G4Material* PVC();
  static G4Material* CuAir(); //crystal tube
  static G4Material* Cu(); // Wires
  static G4Material* LAPPD_Average(); // average LAPPD material to fill space between front and back sections
  static G4Material* StainlessSteel(); 
  static G4Material* Copper(); 
  static G4Material* ESR_Vikuiti(); 
  static G4Material* LAPPD_Window(); // LAPPD quartz window
  static G4Material* LAPPD_MCP(); // material of MCP inside LAPPD
  static G4Material* Epoxy(); // Epoxy resin usually used in PCB materials. For the PCB of the LAPPD backplane
  static G4Material* LAPPD_PCB(); // material of PCB on the backplance of LAPPD
  static G4Material* GarthTypographicAlloy(); // Pb 84% - Sb 12% - 4% Sn - density 10 g/cm3
  static G4Material* Tyvek();
  static G4Material* Pethylene(double scaleFactor);
  static G4Material* PMMA();
  static G4Material* PMMA_Y11(double scaleFactor);
  static G4Material* PMMA_YS2(double scaleFactor);
  static G4Material* PMMA_YS4(double scaleFactor);
  static G4Material* Shashlik_Polystyrene();


  static G4MaterialPropertiesTable* ESR(double esrTransmittance);      // ESR reflector surface
  static G4MaterialPropertiesTable* Teflon(double transmittance);      // Teflon reflector surface
  static G4MaterialPropertiesTable* TiO2();      // TiO2 reflector surface
  static G4MaterialPropertiesTable* ABS_SURF(G4double Reflectivity, G4double specularLobe, G4double specularSpike, G4double backScatter ); // Absorber internal surface
  static G4MaterialPropertiesTable* crystal_depo_SURF(); // crystal lateral surface
  static G4MaterialPropertiesTable* clear_fiber_optical();

  static G4OpticalSurface* surf_Teflon(double transmittance);
  static G4OpticalSurface* surf_ESR();
  static G4OpticalSurface* surf_TiO2();
  static G4OpticalSurface* surf_GapToClearCrystal();
  static G4OpticalSurface* surf_GlassToPhotocathode();
  

  static G4double fromNmToEv(G4double wavelength);
  static G4double fromEvToNm(G4double energy);
  static G4double CalculateSellmeier(int size, G4double indexZero, G4double *nVec, G4double *lVec, G4double wavelength);
};

#endif
