%
%   Test for mexViewshed
%

elData  = imread('D:\Stefan\Cwork\glViewshed\glComputeShader\elevation\z13_2048.png');
elData  = double(elData(:,:,1))*256.0 + double(elData(:,:,2)) + double(elData(:,:,3)/256.0) - 32768.0;
elData(elData<2)    = 0;


DEBUG   = false;

%% Inputs (uniforms)

observerAltitude    = 2.0;
targetAltitude      = 10.0;
lat1                = 32.66;
lon1                = -117.25;
% lat1                = 20.55;
% lon1                = -130.25;
actualRadius        = 6371.009; % km
effectiveRadius     = 4/3*actualRadius;



[tex_h,tex_w]       = size(elData);
imgSize             = [tex_h tex_w]; % y size x size

% Image registration -- upper left corner lat,lon (corresponds to image's
% 0,0) and lower right corner lat,lon (corresponds to image's W,H)
% Note this is approximate, I didn't do due diligence in getting actual
% coordinates of the image
imgBounds       = [ 32.73212635384415, -117.29277687517559, 32.44374242183821, -116.91606950256245  ];


vis = mexViewshed(elData,imgBounds,lat1,lon1,observerAltitude,targetAltitude,actualRadius,effectiveRadius);


%% Hillshade
a = elData(1:end-2,1:end-2);
b = elData(2:end-1,1:end-2);
c = elData(3:end,1:end-2);
d = elData(1:end-2,2:end-1);
e = elData(2:end-1,2:end-1);
f = elData(3:end,2:end-1);
g = elData(1:end-2,3:end);
h = elData(2:end-1,3:end);
i = elData(3:end,3:end);

dz_dy       = ((g + 2*h + i) - (a + 2*b + c)) / (8); dz_dy = cat(2,dz_dy(:,1),dz_dy,dz_dy(:,end)); dz_dy = cat(1,dz_dy(1,:),dz_dy,dz_dy(end,:));
dz_dx       = ((c + 2*f + i) - (a + 2*d + g)) / (8); dz_dx = cat(2,dz_dx(:,1),dz_dx,dz_dx(:,end)); dz_dx = cat(1,dz_dx(1,:),dz_dx,dz_dx(end,:));
z_factor    = 1.0;
slope_rad   = atan(z_factor .* sqrt(dz_dx.*dz_dx + dz_dy.*dz_dy));
aspect_rad  = atan2(dz_dy, -dz_dx);
zenith_rad  = pi/2.0;
azimuth_rad = pi/6.0;
hillshade   = 0.5 + 0.5 * ((cos(zenith_rad) .* cos(slope_rad)) + (sin(zenith_rad) .* sin(slope_rad) .* cos(azimuth_rad - aspect_rad)));


%% Combine hillshade with visbility
outImg          = repmat(hillshade./max(hillshade(:))*0.5,[1 1 3]);
outImg(:,:,1)   = outImg(:,:,1) + double(vis)*0.7;


%%
figure; imagesc(linspace(imgBounds(2),imgBounds(4),size(outImg,2)),linspace(imgBounds(1),imgBounds(3),size(outImg,1)),outImg);
set(gca,'YDir','normal');
hold all;
plot(lon1,lat1,'c.','MarkerSize',12);
