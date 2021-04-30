function visOut = glviewshed(elData,imgBounds,lat1,lon1,observerAltitude,targetAltitude,actualRadius,effectiveRadius)
%
%   Prototype for GL based viewshed type algorithm
%   
%   mexViewshed(elData,imgBounds,lat1,lon1,observerAltitude,targetAltitude,actualRadius,effectiveRadius);

DEBUG   = false;

%% Inputs (uniforms)
% observerAltitude    = 2.0;
% targetAltitude      = 10.0;
% lat1                = 32.56 * pi/180;
% lon1                = -117.25 * pi/180;
% actualRadius        = 6371.009; % km
% effectiveRadius     = 4/3*actualRadius;

lat1                = lat1 * pi/180;
lon1                = lon1 * pi/180;

[tex_h,tex_w]       = size(elData);
imgSize             = [tex_h tex_w]; % y size x size

% Image registration -- upper left corner lat,lon (corresponds to image's
% 0,0) and lower right corner lat,lon (corresponds to image's W,H)
% Note this is approximate, I didn't do due diligence in getting actual
% coordinates of the image
% imgBounds       = [ 32.73212635384415, -117.29277687517559, 32.44374242183821, -116.91606950256245  ] * pi/180;
% imgBounds       = [ 48.73212635384415, -133.29277687517559, 16.44374242183821, -100.91606950256245  ] * pi/180;
imgLat0         = imgBounds(1) * pi/180;
imgLon0         = imgBounds(2) * pi/180;
imgLat2         = imgBounds(3) * pi/180;
imgLon2         = imgBounds(4) * pi/180;


%% Job size
xj      = [ 2:imgSize(2)-1 imgSize(2)*ones(1,imgSize(1)) 2:imgSize(2)-1 1*ones(1,imgSize(1)) ];
yj      = [ 1*ones(1,imgSize(2)-2) 1:imgSize(1) imgSize(1)*ones(1,imgSize(2)-2) 1:imgSize(1) ];

%% Misc (needed in the shader): job index to xj,yj
%{
numRays     = 2*imgSize(1)+2*(imgSize(2)-2);
xjt         = zeros(1,numRays);
yjt         = zeros(1,numRays);
for idx = 0:(numRays-1)
    if idx<imgSize(2)-2
        xjt(idx+1) = 1+idx;
        yjt(idx+1) = 0;
    elseif idx<imgSize(2)-2+imgSize(1)
        xjt(idx+1) = imgSize(2)-1;
        yjt(idx+1) = idx-imgSize(2)+2;
    elseif idx<imgSize(2)-2+imgSize(1)+imgSize(2)-2
        xjt(idx+1) = 3+idx-imgSize(2)-imgSize(1);
        yjt(idx+1) = imgSize(1)-1;
    else
        xjt(idx+1) = 0;
        yjt(idx+1) = idx-2*imgSize(2)-imgSize(1)+4;
    end
end
figure;subplot(2,1,1);plot(xj-1,'s');hold all;plot(xjt,'.');
subplot(2,1,2);plot(yj-1,'s');hold all;plot(yjt,'.');
figure;plot(xjt,yjt,'.'); grid on;
%}

%%
% Observer location to intrinsic units
x1      = (lon1-imgLon0)./(imgLon2-imgLon0)*imgSize(2);
y1      = (lat1-imgLat0)./(imgLat2-imgLat0)*imgSize(1);
x1      = round(x1);
y1      = round(y1);

h1      = elData(y1,x1) + observerAltitude;

% Ray endpoints from instrinsic units to lat,lon
lonj    = imgLon0 + xj./imgSize(2).*(imgLon2-imgLon0);
latj    = imgLat0 + yj./imgSize(1).*(imgLat2-imgLat0);

% 
visOut  = false(size(elData));

% Start processing each ray... this is where GL compute shader will start
nj      = numel(xj);
for ij=1:nj
    
    %%
    lat2    = latj(ij);
    lon2    = lonj(ij);

    % Precompute some parameters for great-circle track way points
    
    % Compute distance between origin and endpoint, in units of radians
    % Using equation from https://edwilliams.org/avform.htm#Dist (the "less
    % subject to rounding error" version)
    d       = 2*asin(sqrt( (sin((lat1-lat2)/2)).^2 + cos(lat1).*cos(lat2)*(sin((lon1-lon2)/2)).^2 ));

    p1a     = cos(lat1)*cos(lon1);
    p1b     = cos(lat2)*cos(lon2);
    p2a     = cos(lat1)*sin(lon1);
    p2b     = cos(lat2)*sin(lon2);
    
    xp      = x1;
    yp      = y1;
    maxAng  = -90;
    flast   = 0;

    % DEBUG
    if DEBUG
    altProfile      = [];
    altAdjProfile   = [];
    elProfile       = [];
    visProfile      = [];
    xyLocation      = [];
    end
    
    for f=0.05:0.05:1 % some uncertainty here in terms of how much to step on each iteration
        % Compute great-circle track way points at fractional f (f=0 is point 1. f=1 is point 2.) 
        A       = sin((1-f)*d)/sin(d);
        B       = sin(f*d)/sin(d);
        x       = A*p1a + B*p1b;
        y       = A*p2a + B*p2b;
        z       = A*sin(lat1)+ B*sin(lat2);
        
        % Segment end location 
        latf    = atan2(z,sqrt(x.^2+y.^2));
        lonf    = atan2(y,x);
        
        % Convert segment end location to intrinsic units (pixels)
        xf      = (lonf-imgLon0)./(imgLon2-imgLon0)*imgSize(2);
        yf      = (latf-imgLat0)./(imgLat2-imgLat0)*imgSize(1);
        
        xf      = round(xf);
        yf      = round(yf);
        
        % Number of pixels in the segment being drawn
        npix    = sqrt( (xf-xp)^2 + (yf-yp)^2 );
        
        % Total segment length in meters
        drpix   = (f-flast)*d*actualRadius*1e3;
        
        % Bresenham Algorithm to draw a line between xp,yp and xf,yf
        dx =  abs(xf - xp); 
        %sx = xp < xf ? 1 : -1;
        if (xp < xf), sx = 1; else, sx = -1; end
        dy = -abs(yf - yp); 
        %sy = yp < yf ? 1 : -1;
        if (yp < yf), sy = 1; else, sy = -1; end
        err = dx + dy; % e2; %/* error value e_xy */

        while (1)
            %visOut(yp,xp)   = true;
            % Range distance within the segment traversed by line drawing algorithm so far
            drng            = (1-sqrt( (xf-xp)^2 + (yf-yp)^2 )/npix)*drpix;
                        
            % Compute the total distance along the ray so far
            gndRng          = (d*flast*actualRadius*1e3+drng);
            
            % Adjust Earth profile altitude to take into account effective radius of the Earth
            %[r,el]         = adjalt(gndRng,elData(yp,xp),effectiveRadius*1e3);
            r               = (effectiveRadius*1e3) + elData(yp,xp);
            phi             = gndRng/(effectiveRadius*1e3);
            rng             = r .* sin(phi);
            el              = r .* cos(phi) - (effectiveRadius*1e3);
            el              = el - h1;
            
            % Compute angles of sight to the terrain
            elAng           = atan( el / rng );
            
            % Compute angles of sight to the elevation above ground level
            testAng         = atan( (el+targetAltitude) / rng );
            
            % Compute visibility state based on largest elevation angle encountered so far
            visOut(yp,xp)   = testAng > maxAng | visOut(yp,xp);
            
            if DEBUG
            visProfile      = [visProfile elAng > maxAng];
            altProfile      = [altProfile elData(yp,xp)];
            altAdjProfile   = [altAdjProfile el];
            elProfile       = [elProfile elAng];
            xyLocation      = [xyLocation [xp;yp]];
            end
            
            maxAng          = max(maxAng,elAng);
            
            if (xp == xf && yp == yf), break; end
            e2 = 2 * err;
            if (e2 >= dy), err = err + dy; xp = xp + sx; end % /* e_xy+e_x > 0 */
            if (e2 <= dx), err = err + dx; yp = yp + sy; end % /* e_xy+e_y < 0 */
        end
        
        flast   = f;
    end
    
    %{
    % DEBUG
    figure,plot(elProfile); hold all; plot(find(visProfile==0),elProfile(visProfile==0),'r.');
    figure,plot(altProfile); hold all; plot(find(visProfile==0),altProfile(visProfile==0),'r.');
    figure;imagesc(elData);
    hold all;
    plot(xyLocation(1,:),xyLocation(2,:),'.');
    
    %}
    
end

% %% Hillshade
% a = elData(1:end-2,1:end-2);
% b = elData(2:end-1,1:end-2);
% c = elData(3:end,1:end-2);
% d = elData(1:end-2,2:end-1);
% e = elData(2:end-1,2:end-1);
% f = elData(3:end,2:end-1);
% g = elData(1:end-2,3:end);
% h = elData(2:end-1,3:end);
% i = elData(3:end,3:end);
% 
% dz_dy       = ((g + 2*h + i) - (a + 2*b + c)) / (8); dz_dy = cat(2,dz_dy(:,1),dz_dy,dz_dy(:,end)); dz_dy = cat(1,dz_dy(1,:),dz_dy,dz_dy(end,:));
% dz_dx       = ((c + 2*f + i) - (a + 2*d + g)) / (8); dz_dx = cat(2,dz_dx(:,1),dz_dx,dz_dx(:,end)); dz_dx = cat(1,dz_dx(1,:),dz_dx,dz_dx(end,:));
% z_factor    = 1.0;
% slope_rad   = atan(z_factor .* sqrt(dz_dx.*dz_dx + dz_dy.*dz_dy));
% aspect_rad  = atan2(dz_dy, -dz_dx);
% zenith_rad  = pi/2.0;
% azimuth_rad = pi/6.0;
% hillshade   = 0.5 + 0.5 * ((cos(zenith_rad) .* cos(slope_rad)) + (sin(zenith_rad) .* sin(slope_rad) .* cos(azimuth_rad - aspect_rad)));


% %% Combine hillshade with visbility
% outImg          = repmat(hillshade./max(hillshade(:))*0.5,[1 1 3]);
% outImg(:,:,1)   = outImg(:,:,1) + visOut*0.7;


% %%
% figure; imagesc(linspace(imgLon0,imgLon2,imgSize(2))*180/pi,linspace(imgLat0,imgLat2,imgSize(1))*180/pi,outImg)
% hold all; plot(lon1*180/pi,lat1*180/pi,'r.','MarkerSize',18);
% plot(lon1*180/pi,lat1*180/pi,'k.','MarkerSize',12);
% set(gca,'YDir','normal');


