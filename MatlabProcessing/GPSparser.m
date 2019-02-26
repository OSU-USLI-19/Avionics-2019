% Run this second!!

%clc;
%clear;
inputFile = fopen('test.txt'); %913
%inputFile = fopen('ParsedData/modified_clean_GPGGA_flight_data.txt'); %687 long
%inputFile = fopen('ParsedData/Fore_section_GPGGA.txt'); %830

nmea_options  =  [  '$GNGGA'
                    '$GNGLL'
                    '$GNGSA'
                    '$GNGSV'
                    '$GNRMC'
                    '$GNVTG'
                    '$GNZDA'
                    '$SDDBS'];
%datalog = []
indexcounter = 1;
%GPRMC read
for i = 1:2254
    inputLine = fgetl(inputFile);
    if(length(inputLine) > 10)
        fields = textscan(inputLine,'%s','delimiter',',');
        %disp(i);
        fields{1}{end+1} = fields{1}{end}(end-1:end);
        fields{1}(end-1) = strtok(fields{1}(end-1), '*');
        case_t = find(strcmp(fields{1}(1), nmea_options),1);
        fields = char(fields{1}); % line delineated into character array

        if(case_t == 1)
            [GNGGAdata,ierr] = nmealineread(inputLine); %using nmealineread, we get
            %coordinates in radians which can be used to plot things. If taken
            %from the field like it is below, the data is still in terms of
            %degrees, minutes and seconds which is undesirable for most
            %calculations

            %%GPVTGdata = nmealineread(fgetl(inputFile));
            %%GPGGAdata = nmealineread(fgetl(inputFile));
            if(ierr == 0)
                arrayplot(indexcounter,1) = GNGGAdata.latitude;
                arrayplot(indexcounter,2) = -GNGGAdata.longitude;
                arrayplot(indexcounter,3) = GNGGAdata.altitude;
                arrayplot(indexcounter,4) = str2double(inputLine(8:17));
                if(i == 1)
                    initialTime = arrayplot(indexcounter,4);
                end
                arrayplot(indexcounter,5) = arrayplot(indexcounter,4) - initialTime;%subtract current time to start at 0
%                 if(arrayplot(indexcounter,5) > 43)
%                     arrayplot(indexcounter, 5) = 43+arrayplot(indexcounter,4) - 185600;
%                 end
                currentlatlon = [arrayplot(indexcounter,1) arrayplot(indexcounter,2)];
                if(i==1)
                    latlonOrigin = [arrayplot(indexcounter,1) arrayplot(indexcounter,2)];
                end
                arrayplot(indexcounter,6) = 3280.8*lldistkm(currentlatlon, latlonOrigin);
                %time = inputLine(8:17);
        %         arrayplot(indexcounter,1) = str2double(fields(3,:)); %lat
        %         arrayplot(indexcounter,2) = str2double(fields(5,:)); %long
        %         arrayplot(indexcounter,3) = str2double(fields(10,:));
                indexcounter= indexcounter + 1;
            end
        end
    end
    
        
    
    
    
end

figure(1);
% /2 is to account for a weird time axis issue we were having on x axis
plot(arrayplot(:,5)/2,arrayplot(:,6)); %drift radius in feet
ylabel('Drift Distance [ft]');
xlabel('Time [s]');

%first GPS coordinate pulled is the reference for everthing else when
%considering the drift distance
%plot(arrayplot2(:,7),arrayplot2(:,6))

%plot(arrayplot5(:,2),arrayplot5(:,1))

%plot gps data
%csvwrite('ForeFullScaleData.csv', arrayplot);
%convert GPS to XYZ

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%old stuff
% R=3959; %miles of earth radius
% counter = 1;
% for j=1:913
%    XYZarray(counter,1)=R*cos(arrayplot(j,1))*cos(arrayplot(j,2));
%    XYZarray(counter,2)=R*cos(arrayplot(j,1))*sin(arrayplot(j,2));
%    XYZarray(counter,3)=R*sin(arrayplot(j,1));
%    counter=counter+1;
% end
% 
% %plot3(XYZarray(:,1),XYZarray(:,2),XYZarray(:,3));
% 
% %plots xy path of rocket
% %plot(XYZarray(:,1),XYZarray(:,2));
% fclose(inputFile);
% 
% x1 = XYZarray(1,1);
% y1 = XYZarray(1,2);
% 
% for k=1:913
%    driftRad(k) = sqrt((XYZarray(k,1) - x1)^2 + (XYZarray(k,1) - x1)^2);
%    driftRadFeet(k) = 5280*(driftRad(k)/100);
% end

%figure(2);
%plot(driftRad); %drift radius in miles*100
%figure(1);
%plot(arrayplot(:,5),driftRadFeet) %drift radius in feet



