inputFile = fopen('aft.txt');

outputFile = fopen('output.txt', 'w');

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