#!/usr/bin/python

import numpy as np

def dispersion(T, h):    
    L0 = 9.81*T**2/(2.*np.pi)
    L = L0
    for i in range(0,100):
        Lnew = L0 * np.tanh(2.*np.pi/L*h)
        if(abs(Lnew-L)<0.001):
            L = Lnew
            break
        L = Lnew
    return L

## Flap wavemaker data ##
H = 0.1
T = 3.0
h = 0.4
phase0 = 0.
direction = 15.

nPaddles = 10
bLims = [0., 5.]

t0 = 0.
tEnd = 30.
dt = 0.05
########################

# Calculations
L = dispersion(T, h)
k = 2.*np.pi/L
w = 2.*np.pi/T

times = np.linspace(t0, tEnd, round((tEnd-t0)/dt)+1)
coords = np.linspace(bLims[0], bLims[1], nPaddles+1)
coords = coords[:-1] + np.diff(coords)/2.

HoS = 4. * np.sinh(k*h)/(k*h) * (k*h*np.sinh(k*h) - np.cosh(k*h) + 1.)/(np.sinh(2.*k*h) + 2.*k*h)
S = H/HoS

# Export
fid = open('wavemakerMovement.txt', 'w')

fid.write('wavemakerType   Flap;\n')
fid.write('tSmooth         1.5;\n')
fid.write('genAbs          0;\n\n')

fid.write('timeSeries {0}(\n'.format( len(times) ))
for t in times:
    fid.write('{0}\n'.format(t))
fid.write(');\n\n'.format( len(times) ))

fid.write('paddleTilt {0}(\n'.format( nPaddles ))
for i in range(0, nPaddles):
    fid.write('{0}(\n'.format( len(times) ))
    for t in times:
        x = S/2. * np.cos(-w*t + np.pi/2. + phase0 + 2.*np.pi*coords[i]/L*np.sin(direction*np.pi/180.) )
        x = np.arctan(x/h)
        x = x*180./np.pi
        fid.write('{0}\n'.format(x))       
    fid.write(')\n')
fid.write(');\n')

fid.close()
