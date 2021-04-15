function vis = mexViewshed(Z,R,lat1,lon1,obsAlt,tgtAlt,re,reEff)
%
%
%

mex mexViewshed.cpp ../context.cpp ../shader.cpp ../gl/glad.c -I.. -lopengl32

