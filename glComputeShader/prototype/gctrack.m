function [latf,lonf] = gctrack(lat1,lon1,lat2,lon2,f)
% [latf,lonf] = gctrack(lat1,lon1,lat2,lon2,f)
%
% Source: https://edwilliams.org/avform.htm#Intermediate

%         A=sin((1-f)*d)/sin(d)
%         B=sin(f*d)/sin(d)
%         x = A*cos(lat1)*cos(lon1) +  B*cos(lat2)*cos(lon2)
%         y = A*cos(lat1)*sin(lon1) +  B*cos(lat2)*sin(lon2)
%         z = A*sin(lat1)           +  B*sin(lat2)
%         lat=atan2(z,sqrt(x^2+y^2))
%         lon=atan2(y,x)

lat1    = lat1*pi/180;
lon1    = lon1*pi/180;
lat2    = lat2*pi/180;
lon2    = lon2*pi/180;

d       = 2*asin(sqrt((sin((lat1-lat2)/2)).^2 + cos(lat1).*cos(lat2)*(sin((lon1-lon2)/2)).^2));

A       = sin((1-f)*d)/sin(d);
B       = sin(f*d)/sin(d);
x       = A*cos(lat1)*cos(lon1) +  B*cos(lat2)*cos(lon2);
y       = A*cos(lat1)*sin(lon1) +  B*cos(lat2)*sin(lon2);
z       = A*sin(lat1)           +  B*sin(lat2);
latf    = atan2(z,sqrt(x.^2+y.^2));
lonf    = atan2(y,x);

latf    = latf*180/pi;
lonf    = lonf*180/pi;

%{
f   = linspace(0,1,101);
[latf,lonf] = gctrack(42.344817170747376, -71.0318196771081,51.3955116420648, -0.12459643578480432,f);
%}