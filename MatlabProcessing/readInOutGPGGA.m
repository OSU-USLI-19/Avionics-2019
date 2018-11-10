inputFile = fopen('parsedData/ForeDataShort_pad2roverEjection.txt');

outputFile = fopen('parsedData/Fore_section_pad2roverEjection_GPGGA.txt', 'w');

for i=1:315821  %need to figure out how to do this automatically
   inputLine = fgetl(inputFile); 
    
    if(length(inputLine) > 5)
        type = inputLine(1:6);
        
        if(type == '$GPGGA')
            if(length(inputLine) > 50)
                
                fprintf(outputFile, '%s\n', inputLine);
            end
        end
    end
        
end
fclose(inputFile);