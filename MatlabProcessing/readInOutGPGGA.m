% Run this first!!

inputFile = fopen('DATALOG_filtered.txt'); % Enter input filename here

outputFile = fopen('test.txt', 'w'); % Enter output filename here

for i=1:315821  %need to figure out how to do this automatically
   inputLine = fgetl(inputFile); 
    
    if(length(inputLine) > 5)
        type = inputLine(1:6);
        
        if(type == '$GNGGA')
            if(length(inputLine) > 50)
                
                fprintf(outputFile, '%s\n', inputLine);
            end
        end
    end
        
end
fclose(inputFile);