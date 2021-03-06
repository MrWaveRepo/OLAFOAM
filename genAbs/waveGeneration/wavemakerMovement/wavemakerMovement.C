/*---------------------------------------------------------------------------*\
License
    This file is part of olaFoam Project.

    olaFoam is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    olaFoam is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with olaFoam.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*\
| olaFoam Project                                       ll                    |
|                                                       l l                   |
|   Coder: Pablo Higuera Caubilla                 ooo   l l     aa            |
|   Bug reports: olafoamwaves@gmail.com          o   o  l l    a  a           |
|                                                o   o  ll   l a  aa  aa      |
|                                                 ooo    llll   aa  aa        |
|                                                                             |
|                                                FFFFF OOOOO AAAAA M   M      |
|                                                F     O   O A   A MM MM      |
|                                                FFFF  O   O AAAAA M M M      |
|                                                F     O   O A   A M   M      |
|                                                F     OOOOO A   A M   M      |
|   -----------------------------------------------------------------------   |
| References:                                                                 |
|                                                                             |
| - Realistic wave generation and active wave absorption for Navier-Stokes    |
|    models: Application to OpenFOAM.                                         |
|    Higuera, P., Lara, J.L. and Losada, I.J. (2013)                          |
|    Coastal Engineering, Vol. 71, 102-118.                                   |
|    http://dx.doi.org/10.1016/j.coastaleng.2012.07.002                       |
|                                                                             |
| - Simulating coastal engineering processes with OpenFOAM                    |
|    Higuera, P., Lara, J.L. and Losada, I.J. (2013)                          |
|    Coastal Engineering, Vol. 71, 119-134.                                   |
|    http://dx.doi.org/10.1016/j.coastaleng.2012.06.002                       |
|                                                                             |
| - Three-dimensional numerical wave generation with moving boundaries        |
|    Higuera, P., Losada, I.J. and Lara, J.L. (2015)                          |
|    Coastal Engineering, Vol. 101, 35-47.                                    |
|    http://dx.doi.org/10.1016/j.coastaleng.2015.04.003                       |
|                                                                             |
\*---------------------------------------------------------------------------*/

#include "wavemakerMovement.H"
#include "addToRunTimeSelectionTable.H"

#include "pointFields.H"

#include "fvPatchFieldMapper.H"
#include "surfaceFields.H"
#include "volFields.H"

#if OFFLAVOUR == 1
    #include "PointPatchFieldMapper.H"
#else
    #include "pointPatchFieldMapper.H"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

makePointPatchTypeField
(
    pointPatchVectorField,
    wavemakerMovement
);

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

wavemakerMovement::
wavemakerMovement
(
    const pointPatch& p,
    const DimensionedField<vector, pointMesh>& iF
)
:
    #if OFFLAVOUR == 1
        fixedValuePointPatchVectorField(p, iF),
    #else
        fixedValuePointPatchField<vector>(p, iF),
    #endif
    wavemakerDictName_("wavemakerMovementDict"),
    wavemakerType_("Piston"),
    timeSeries_( List<scalar> (1, -1.0) ),
    nPaddles_(1),
    paddlePosition_( List<List<scalar> > (1,List<scalar> (1, -1.0)) ),
    paddleTilt_( List<List<scalar> > (1,List<scalar> (1, -1.0)) ),
    paddleEta_( List<List<scalar> > (1,List<scalar> (1, -1.0)) ),
    initialWaterDepth_(-1),
    meanAngle_(vector (0,0,0)),
    hingeHeight_(999.0),
    hingeLocation_(999.0),
    genAbs_(false),
    DPS_(List<bool> (1, false)),
    maxStroke_(999.0),
    DPST_(25),
    DPSsign_(List<scalar> (nPaddles_, 1.0)),
    DPStIni_(List<scalar> (nPaddles_, -1.0)),
    instDPSCorrection_(List<scalar> (nPaddles_, 0.0)),
    cumDPSCorrection_(List<scalar> (nPaddles_, 0.0)),
    cumAbsCorrection_(List<scalar> (nPaddles_, 0.0)),
    tiltOld_(List<scalar> (nPaddles_, 0.0)),
    tSmooth_(-1),
    tuningFactor_(1),
    allCheck_(false)
{}


wavemakerMovement::
wavemakerMovement
(
    const wavemakerMovement& ptf,
    const pointPatch& p,
    const DimensionedField<vector, pointMesh>& iF,
    #if OFFLAVOUR == 1
        const PointPatchFieldMapper& mapper
    #else
        const pointPatchFieldMapper& mapper
    #endif
)
:
    #if OFFLAVOUR == 1
        fixedValuePointPatchVectorField(ptf, p, iF, mapper),
    #else
        fixedValuePointPatchField<vector>(ptf, p, iF, mapper),
    #endif
    wavemakerDictName_(ptf.wavemakerDictName_),
    wavemakerType_(ptf.wavemakerType_),
    timeSeries_(ptf.timeSeries_),
    nPaddles_(ptf.nPaddles_),
    paddlePosition_(ptf.paddlePosition_),
    paddleTilt_(ptf.paddleTilt_),
    paddleEta_(ptf.paddleEta_),
    initialWaterDepth_(ptf.initialWaterDepth_),
    meanAngle_(ptf.meanAngle_),
    hingeHeight_(ptf.hingeHeight_),
    hingeLocation_(ptf.hingeLocation_),
    genAbs_(ptf.genAbs_),
    DPS_(ptf.DPS_),
    maxStroke_(ptf.maxStroke_),
    DPST_(ptf.DPST_),
    DPSsign_(ptf.DPSsign_),
    DPStIni_(ptf.DPStIni_),
    instDPSCorrection_(ptf.instDPSCorrection_),
    cumDPSCorrection_(ptf.cumDPSCorrection_),
    cumAbsCorrection_(ptf.cumAbsCorrection_),
    tiltOld_(ptf.tiltOld_),
    tSmooth_(ptf.tSmooth_),
    tuningFactor_(ptf.tuningFactor_),
    allCheck_(ptf.allCheck_)
{}


wavemakerMovement::
wavemakerMovement
(
    const pointPatch& p,
    const DimensionedField<vector, pointMesh>& iF,
    const dictionary& dict,
    const bool valueRequired
)
:
    #if OFFLAVOUR == 1
        fixedValuePointPatchVectorField(p, iF, dict),
    #else
        fixedValuePointPatchField<vector>(p, iF, dict, valueRequired),
    #endif
    wavemakerDictName_(dict.lookupOrDefault<word>("wavemakerDictName", "wavemakerMovementDict")),
    wavemakerType_(dict.lookupOrDefault<word>("wavemakerType", "Piston")),
    timeSeries_( dict.lookupOrDefault("timeSeries", List<scalar> (1, -1.0)) ),
    nPaddles_(dict.lookupOrDefault<label>("nPaddles", 1)),
    paddlePosition_( dict.lookupOrDefault("paddlePosition", List<List<scalar> > (1,List<scalar> (1, -1.0)) )),
    paddleTilt_( dict.lookupOrDefault("paddleTilt", List<List<scalar> > (1,List<scalar> (1, -1.0)) )),
    paddleEta_( dict.lookupOrDefault("paddleEta", List<List<scalar> > (1,List<scalar> (1, -1.0)) )),
    initialWaterDepth_(dict.lookupOrDefault<scalar>("initialWaterDepth", -1 )),
    meanAngle_(dict.lookupOrDefault("meanAngle", vector (0,0,0) )),
    hingeHeight_(dict.lookupOrDefault<scalar>("hingeHeight", 999.0)),
    hingeLocation_(dict.lookupOrDefault<scalar>("hingeLocation", 999.0)),
    genAbs_(dict.lookupOrDefault<bool>("genAbs", false )),
    DPS_(dict.lookupOrDefault("DPS", List<bool> (1, false) )),
    maxStroke_(dict.lookupOrDefault<scalar>("maxStroke", 999.0)),
    DPST_(dict.lookupOrDefault<scalar>("DPST", 25)),
    DPSsign_(dict.lookupOrDefault("DPSsign", List<scalar> (nPaddles_, 0.0))),
    DPStIni_(dict.lookupOrDefault("DPStIni", List<scalar> (nPaddles_, -1.0))),
    instDPSCorrection_(dict.lookupOrDefault("instDPSCorrection", List<scalar> (nPaddles_, 0.0))),
    cumDPSCorrection_(dict.lookupOrDefault("cumDPSCorrection", List<scalar> (nPaddles_, 0.0))),
    cumAbsCorrection_(dict.lookupOrDefault("cumAbsCorrection", List<scalar> (nPaddles_, 0.0))),
    tiltOld_(dict.lookupOrDefault("tiltOld", List<scalar> (nPaddles_, 0.0))),
    tSmooth_(dict.lookupOrDefault<scalar>("tSmooth", -1)),
    tuningFactor_(dict.lookupOrDefault<scalar>("tuningFactor", 1)),
    allCheck_(dict.lookupOrDefault<bool>("allCheck", false ))
{}


wavemakerMovement::
wavemakerMovement
(
    const wavemakerMovement& ptf
)
:
    #if OFFLAVOUR == 1
        fixedValuePointPatchVectorField(ptf),
    #else
        fixedValuePointPatchField<vector>(ptf),
    #endif
    wavemakerDictName_(ptf.wavemakerDictName_),
    wavemakerType_(ptf.wavemakerType_),
    timeSeries_(ptf.timeSeries_),
    nPaddles_(ptf.nPaddles_),
    paddlePosition_(ptf.paddlePosition_),
    paddleTilt_(ptf.paddleTilt_),
    paddleEta_(ptf.paddleEta_),
    initialWaterDepth_(ptf.initialWaterDepth_),
    meanAngle_(ptf.meanAngle_),
    hingeHeight_(ptf.hingeHeight_),
    hingeLocation_(ptf.hingeLocation_),
    genAbs_(ptf.genAbs_),
    DPS_(ptf.DPS_),
    maxStroke_(ptf.maxStroke_),
    DPST_(ptf.DPST_),
    DPSsign_(ptf.DPSsign_),
    DPStIni_(ptf.DPStIni_),
    instDPSCorrection_(ptf.instDPSCorrection_),
    cumDPSCorrection_(ptf.cumDPSCorrection_),
    cumAbsCorrection_(ptf.cumAbsCorrection_),
    tiltOld_(ptf.tiltOld_),
    tSmooth_(ptf.tSmooth_),
    tuningFactor_(ptf.tuningFactor_),
    allCheck_(ptf.allCheck_)
{}


wavemakerMovement::
wavemakerMovement
(
    const wavemakerMovement& ptf,
    const DimensionedField<vector, pointMesh>& iF
)
:
    #if OFFLAVOUR == 1
        fixedValuePointPatchVectorField(ptf, iF),
    #else
        fixedValuePointPatchField<vector>(ptf, iF),
    #endif
    wavemakerDictName_(ptf.wavemakerDictName_),
    wavemakerType_(ptf.wavemakerType_),
    timeSeries_(ptf.timeSeries_),
    nPaddles_(ptf.nPaddles_),
    paddlePosition_(ptf.paddlePosition_),
    paddleTilt_(ptf.paddleTilt_),
    paddleEta_(ptf.paddleEta_),
    initialWaterDepth_(ptf.initialWaterDepth_),
    meanAngle_(ptf.meanAngle_),
    hingeHeight_(ptf.hingeHeight_),
    hingeLocation_(ptf.hingeLocation_),
    genAbs_(ptf.genAbs_),
    DPS_(ptf.DPS_),
    maxStroke_(ptf.maxStroke_),
    DPST_(ptf.DPST_),
    DPSsign_(ptf.DPSsign_),
    DPStIni_(ptf.DPStIni_),
    instDPSCorrection_(ptf.instDPSCorrection_),
    cumDPSCorrection_(ptf.cumDPSCorrection_),
    cumAbsCorrection_(ptf.cumAbsCorrection_),
    tiltOld_(ptf.tiltOld_),
    tSmooth_(ptf.tSmooth_),
    tuningFactor_(ptf.tuningFactor_),
    allCheck_(ptf.allCheck_)
{}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

wavemakerMovement::
~wavemakerMovement()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void wavemakerMovement::updateCoeffs()
{
    if (this->updated())
    {
        return;
    }

    // PHC //
    Info << "Point displacement BC on patch " << this->patch().name() << endl;

    // Variables
    const scalar g = 9.81;

    const volScalarField& alpha = 
        db().lookupObject<volScalarField>(alphaName());
    const fvMesh& mesh = alpha.mesh();

    label patchId = mesh.boundaryMesh().findPatchID(this->patch().name());

    const scalarField cellSurface = mesh.magSf().boundaryField()[patchId]; // Surface of the patch face
    const vectorField cellVector = 
        mesh.Sf().boundaryField()[patchId]/cellSurface; // Unit vector of the patch face
    const scalarField alphaCell = alpha.boundaryField()[patchId]; // Alpha of the patch face


    scalarField patchD = mesh.Cf().boundaryField()[patchId].component(1);

    const scalar yMin = gMin(this->patch().localPoints().component(1)); // Min Y of the patch
    const scalar yMax = gMax(this->patch().localPoints().component(1)); // Max Y of the patch
    const scalar ySpan = yMax-yMin;

    const scalar zMin = gMin(this->patch().localPoints().component(2)); // Min Z of the patch
    const scalar zMax = gMax(this->patch().localPoints().component(2)); // Max Z of the patch
    const scalar zSpan = zMax-zMin;

    // Define dictionary
    IOdictionary wavemakerMovementDict
    (
        IOobject
        (
            wavemakerDictName_,
            this->db().time().constant(),
            this->db(),
            IOobject::MUST_READ,
            IOobject::NO_WRITE
        )
    );

    if (!allCheck_) // Just the first time
    {
        #include "firstTimeCheck.H"
    }

    // Time interpolation
    scalar currTime = this->db().time().value();
    scalar timeMult = tuningFactor_;

    if ( tSmooth_ > 0.0 && currTime < tSmooth_ )
    {
        timeMult = timeMult*currTime/tSmooth_;
    }

    if( currTime < min(timeSeries_) )
    {
        FatalError
            << "The first time in timeSeries should be <= startTime."
            << exit(FatalError);
    }
    else if( currTime > max(timeSeries_) )
    {
        FatalError
            << "The time series is not long enough."
            << exit(FatalError);
    }

    // Interpolate displacement, eta and tilt
    label indexF = 0;
    scalarList dispInterp = scalarList(nPaddles_, 0.0);
    scalarList etaInterp = scalarList(nPaddles_, 0.0);
    scalarList tiltInterp = scalarList(nPaddles_, 0.0);

    forAll( timeSeries_, contador )
    {
        if ( timeSeries_[contador] >= currTime )
        {
            indexF = contador;
            break;
        }
    }

    for(int i=0; i<nPaddles_; i++)
    {
        if ( indexF == 0)
        {
            if ( wavemakerType_ == "Piston" )
            {
                dispInterp[i] = timeMult * paddlePosition_[i][0];
                
                if ( genAbs_ )
                { 
                    etaInterp[i] = timeMult * paddleEta_[i][0];
                }
            }
            else if ( wavemakerType_ == "Flap" )
            {
                tiltInterp[i] = timeMult * paddleTilt_[i][0];
            }
            else if ( wavemakerType_ == "Mixed" )
            {
                dispInterp[i] = timeMult * paddlePosition_[i][0];
                tiltInterp[i] = timeMult * paddleTilt_[i][0];
            }
        }
        else
        {
            if ( wavemakerType_ == "Piston" )
            {
                dispInterp[i] = 
                    timeMult *
                    linInterp
                    (
                        timeSeries_[indexF-1],
                        timeSeries_[indexF],
                        paddlePosition_[i][indexF-1],
                        paddlePosition_[i][indexF],
                        currTime
                    );
                
                if ( genAbs_ )
                {
                    etaInterp[i] = 
                        timeMult *
                        linInterp
                        (
                            timeSeries_[indexF-1],
                            timeSeries_[indexF],
                            paddleEta_[i][indexF-1],
                            paddleEta_[i][indexF],
                            currTime
                        );
                }
            }
            else if ( wavemakerType_ == "Flap" )
            {
                tiltInterp[i] = 
                    timeMult *
                    linInterp
                    (
                        timeSeries_[indexF-1],
                        timeSeries_[indexF],
                        paddleTilt_[i][indexF-1],
                        paddleTilt_[i][indexF],
                        currTime
                    );
            }
            else if ( wavemakerType_ == "Mixed" )
            {
                dispInterp[i] =
                    timeMult *
                    linInterp
                    (
                        timeSeries_[indexF-1],
                        timeSeries_[indexF],
                        paddlePosition_[i][indexF-1],
                        paddlePosition_[i][indexF],
                        currTime
                    );
                tiltInterp[i] =
                    timeMult *
                    linInterp
                    (
                        timeSeries_[indexF-1],
                        timeSeries_[indexF],
                        paddleTilt_[i][indexF-1],
                        paddleTilt_[i][indexF],
                        currTime
                    );
            }
        }
    }

    // Active absorption correction - Only working for piston paddles
    if ( genAbs_ )
    {
        // Measure water depth
        scalarList measuredWaterLevel =
            calcWL( alphaCell, patchD, cellSurface, yMin, ySpan, zSpan );

        // Calculate correction
        scalar deltaT = db().time().deltaTValue();

        for(int i=0; i<nPaddles_; i++)
        {
            scalar expectedWaterLevel = initialWaterDepth_ + etaInterp[i];
            cumAbsCorrection_[i] -=
                timeMult*deltaT*(measuredWaterLevel[i] - expectedWaterLevel)*
                sqrt(g/initialWaterDepth_); // !Angle correction
        }
    }

    // Drift Prevention System (DPS)
    if
    (
        maxStroke_ != 999.0 && 
        (wavemakerType_ == "Piston" || wavemakerType_ == "Mixed")
    ) 
    {
        for(int i=0; i<nPaddles_; i++)
        {
            if(!DPS_[i]) 
            {
                // Test if need to connect
                scalar dispAux = dispInterp[i] +
                    cumAbsCorrection_[i] + cumDPSCorrection_[i];
                if( dispAux > 0.8*maxStroke_ )
                {
                    DPS_[i] = true;
                    DPSsign_[i] = -1.0;
                    DPStIni_[i] = currTime;
                }
                else if( dispAux < -0.8*maxStroke_ )
                {
                    DPS_[i] = true;
                    DPSsign_[i] = 1.0;
                    DPStIni_[i] = currTime;
                }
            }

            if(DPS_[i]) // If active
            {
                if( currTime-DPStIni_[i] < DPST_ )
                {
                    instDPSCorrection_[i] =
                        DPSsign_[i] *
                        DPSramp(maxStroke_, currTime-DPStIni_[i], DPST_);
                }
                else
                {
                    instDPSCorrection_[i] = 0.0;
                    cumDPSCorrection_[i] += DPSsign_[i] * maxStroke_;
                    DPS_[i] = false;
                }
            }
        }
    }

    // Displacement of the paddles
    scalarList displacements = scalarList(nPaddles_, 0.0);
    scalarList paddleCenters = scalarList(nPaddles_, 0.0);
    scalar paddleWidth = ySpan/nPaddles_;

    for(int i=0; i<nPaddles_; i++)
    {
        paddleCenters[i] = yMin + (0.5 + i) * paddleWidth;
        displacements[i] = 
            dispInterp[i] + cumAbsCorrection_[i] +
            instDPSCorrection_[i] + cumDPSCorrection_[i];
    }    

    if ( wavemakerType_ == "Piston" || wavemakerType_ == "Mixed" )
    {
        for(int i=0; i<nPaddles_; i++)
        {
            displacements[i] =
                dispInterp[i] + cumAbsCorrection_[i] +
                instDPSCorrection_[i] + cumDPSCorrection_[i];
        }  

        Info << "Displacement Paddles_" << this->patch().name()
            << " => " << displacements << endl;
    }

    if ( wavemakerType_ == "Flap" || wavemakerType_ == "Mixed" )
    {
        Info << "Tilting Paddles_" << this->patch().name() << " => "
            << tiltInterp << endl;
    }

    // Interpolation to the points
    vectorField auxPoints = this->patch().localPoints();
    vectorField newPoints = 0.0*auxPoints;

    if ( wavemakerType_ == "Piston" )
    {
        forAll( auxPoints, pInd )
        {
            // Interpolation procedure
            scalar pDisp =
                paddleCosInterp
                (
                    paddleCenters,
                    displacements,
                    auxPoints[pInd].component(1)
                );

            auxPoints[pInd] = pDisp * meanAngle_;
        }
    }
    else if ( wavemakerType_ == "Flap" )
    {
        forAll( auxPoints, pInd )
        { 
            vector point = auxPoints[pInd];
            
            if ( point.component(2) > hingeHeight_ )
            {
                label padI = paddleIndex(point.component(1), yMin, ySpan);

                // Correct Z component to vertical value
                scalar arm = 
                    sqrt
                    (
                        sqr(point.component(0)-hingeLocation_) +
                        sqr(point.component(2)-hingeHeight_)
                    );
                scalar pAng =
                    paddleTiltCosInterp
                    (
                        paddleCenters,
                        tiltInterp,
                        (zMax-hingeHeight_)*sin(deg2rad(tiltInterp)),
                        auxPoints[pInd].component(1)
                    );
                scalar myAng = deg2rad( pAng );

                // New X component
                newPoints[pInd].component(0) = hingeLocation_ + arm*sin(myAng);
                newPoints[pInd].component(1) = 0.0;
                newPoints[pInd].component(2) = -arm*(1.-cos(myAng));
                    //-arm*sin(myAng)*sin(myAng)/cos(myAng);
            }
        }
        
        auxPoints = newPoints;
    }

// tiltInterp, tiltOld_, hingeLocation_

    #if OFFLAVOUR == 1
        Field<vector>::operator=(auxPoints);
        fixedValuePointPatchVectorField::updateCoeffs();
    #else
        (*this) == (auxPoints);
        this->fixedValuePointPatchField<vector>::updateCoeffs();
    #endif
}

void wavemakerMovement::write(Ostream& os) const
{
    #if OFFLAVOUR == 1
        fixedValuePointPatchVectorField::write(os);
    #else
        fixedValuePointPatchField<vector>::write(os);
    #endif
    
    os.writeKeyword("allCheck") << allCheck_ << token::END_STATEMENT << nl;
    os.writeKeyword("wavemakerDictName") << wavemakerDictName_
        << token::END_STATEMENT << nl;
    os.writeKeyword("wavemakerType") << wavemakerType_
        << token::END_STATEMENT << nl;

    os.writeKeyword("nPaddles") << nPaddles_ << token::END_STATEMENT << nl;
    os.writeKeyword("initialWaterDepth") << initialWaterDepth_
        << token::END_STATEMENT << nl;
    os.writeKeyword("meanAngle") << meanAngle_ << token::END_STATEMENT << nl;

    timeSeries_.writeEntry("timeSeries", os);

    if ( tSmooth_ != -1.0 )
    {
        os.writeKeyword("tSmooth") << tSmooth_ << token::END_STATEMENT << nl;
    }

    if ( tuningFactor_ != 1.0 )
    {
        os.writeKeyword("tuningFactor") << tuningFactor_
            << token::END_STATEMENT << nl;
    }

    if ( wavemakerType_ == "Piston" )
    {
        // paddlePosition_.writeEntry("paddlePosition", os);
        os.writeKeyword("paddlePosition") << paddlePosition_
            << token::END_STATEMENT << nl; // May not work for binary encoding

        if ( genAbs_ )
        {
            os.writeKeyword("genAbs") << genAbs_ << token::END_STATEMENT << nl;
            cumAbsCorrection_.writeEntry("cumAbsCorrection", os);
            // paddleEta_.writeEntry("paddleEta", os);
            os.writeKeyword("paddleEta") << paddleEta_
                << token::END_STATEMENT << nl; // May not work for binary encoding
        }
    }
    else if ( wavemakerType_ == "Flap" )
    {
        os.writeKeyword("paddleTilt") << paddleTilt_
            << token::END_STATEMENT << nl; // May not work for binary encoding
        os.writeKeyword("hingeHeight") << hingeHeight_
            << token::END_STATEMENT << nl;
        os.writeKeyword("hingeLocation") << hingeLocation_
            << token::END_STATEMENT << nl;

        tiltOld_.writeEntry("tiltOld", os);
    }
    else if ( wavemakerType_ == "Mixed" )
    {
        os.writeKeyword("paddlePosition") << paddlePosition_
            << token::END_STATEMENT << nl; // May not work for binary encoding
        os.writeKeyword("paddleTilt") << paddleTilt_
            << token::END_STATEMENT << nl; // May not work for binary encoding

        os.writeKeyword("hingeHeight") << hingeHeight_
            << token::END_STATEMENT << nl;
        tiltOld_.writeEntry("tiltOld", os);
    }

    if ( wavemakerType_ == "Piston" || wavemakerType_ == "Mixed" )
    {
        if ( maxStroke_ != 999.0 ) // DPS active
        {
            DPS_.writeEntry("DPS", os);
            os.writeKeyword("maxStroke") << maxStroke_
                << token::END_STATEMENT << nl;
            os.writeKeyword("DPST") << DPST_ << token::END_STATEMENT << nl;
            cumDPSCorrection_.writeEntry("cumDPSCorrection", os);
            DPSsign_.writeEntry("DPSsign", os);
            DPStIni_.writeEntry("DPStIni", os);
            instDPSCorrection_.writeEntry("instDPSCorrection", os);
        }
    }

    //writeEntry("value", os);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //

