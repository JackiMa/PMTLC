#ifndef MY_PHYSICAL_VOLUME_HH
#define MY_PHYSICAL_VOLUME_HH

#include "G4PVPlacement.hh"
#include "G4ThreeVector.hh"
#include "G4RotationMatrix.hh"
#include "G4Transform3D.hh"

class MyPhysicalVolume : public G4PVPlacement {
private:
    G4ThreeVector absolutePosition;  
    G4RotationMatrix* absoluteRotation;

public:
    // 构造函数
    MyPhysicalVolume(G4RotationMatrix* pRot, const G4ThreeVector& tlate, 
                     const G4String& pName, G4LogicalVolume* pLogical, 
                     MyPhysicalVolume* pMotherVolume, G4bool pMany, 
                     G4int pCopyNo, G4bool pSurfChk = false)
        : G4PVPlacement(pRot, tlate, pName, pLogical, 
                        pMotherVolume ? static_cast<G4VPhysicalVolume*>(pMotherVolume) : nullptr, 
                        pMany, pCopyNo, pSurfChk)
    {
        absolutePosition = pMotherVolume ? pMotherVolume->GetAbsolutePosition() + tlate : tlate;
        
        if (pMotherVolume) {
            absoluteRotation = new G4RotationMatrix(*(pMotherVolume->GetAbsoluteRotation()));
            if (pRot) {
                *absoluteRotation *= *pRot;
            }
        } else {
            absoluteRotation = pRot ? new G4RotationMatrix(*pRot) : new G4RotationMatrix();
        }
        
    }

    // 获取绝对位置
    G4ThreeVector GetAbsolutePosition() const {
        return absolutePosition;
    }

    // 获取相对于世界体的绝对旋转
    G4RotationMatrix* GetAbsoluteRotation() const {
        return absoluteRotation;
    }

    // 获取相对于世界体的绝对Transform
    G4Transform3D GetAbsoluteTransform() const {
        return G4Transform3D(*absoluteRotation, absolutePosition);
    }
};

#endif // MY_PHYSICAL_VOLUME_HH