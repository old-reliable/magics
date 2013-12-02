# importing Magics module
from Magics.macro import *

# Definitions of the projection using a dictionary to 
# organise the informations
projection = { "name": "projection6",
               "project" : "goode",
               "text" : "Mollweide projection ( using Proj4 Package)",
                "map" : mmap(
					subpage_map_area_definition =  "full",
                    subpage_map_projection =  "mollweide")
             }

# Setting of the output file name
output = output(output_name=projection["name"],
				output_formats = ['png'],
				output_name_first_page_number = "off"
			    )

# Setting the coordinates of the geographical area
map=projection["map"]


# Setting the coastlines attributes
coast = mcoast( 
			map_coastline_colour = "grey",
			map_coastline_land_shade =  "on", 
			map_coastline_land_shade_colour =  "cream", 
			map_grid =  "on", 
			map_grid_colour = "grey", 
			map_coastline_sea_shade  ="on", 
			map_coastline_sea_shade_colour=  "white", 
			map_label  ="off")

#Setting of the title
text = mtext( text_font_size = '0.7', 
		text_justification = "left",
		text_lines =  [projection["text"]])

# Plot the result
plot(output, map, coast, text)

# For documentation purposes
tofortran(projection["name"], output, map, coast, text)
tomv4(projection["name"], map)
tohtml(projection["name"],  map)
