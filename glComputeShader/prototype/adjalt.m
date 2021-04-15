function [x,z] = adjalt(arclen,zin,appr)
% adjust terrain altitude for the curvature of the sphere
r   = appr + zin;
phi = arclen/appr;
x   = r .* sin(phi);
z   = r .* cos(phi) - appr;
