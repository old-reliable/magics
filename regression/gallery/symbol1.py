
#importing Magics module
from Magics.macro import *


ref = 'symbol1'
#Setting of the output file name
output = output(output_formats = ['png'], 
		output_name_first_page_number = "off",
		output_name = ref)

#Setting the coordinates of the geographical area
#Here we use Europe
area = mmap(subpage_lower_left_latitude= 40.,
            subpage_lower_left_longitude= -20.,
			subpage_upper_right_latitude= 65.,
			subpage_upper_right_longitude= 10.)

#Background Coastlines 
background = mcoast( map_coastline_sea_shade_colour = "white",
                     map_coastline_land_shade_colour = "cream",
                     map_grid = "on",
                     map_coastline_land_shade = "on",
                     map_coastline_sea_shade = "on",
                     map_label = "on",
                     map_coastline_colour = "tan")



#Import the input data 
input =  mgeo(geo_input_file_name = "input.geo")

#Define the symbol plotting
symbol = msymb( legend = "off", 
			symbol_type =  "marker",
		    symbol_colour =  "red", 
			symbol_height =  1.,
			symbol_marker_index = 15)

lines =["Big Red dots..."]

title = mtext({
           "text_lines" : lines,
           "text_html" : "true",
		   "text_justification": "left",
		   "text_font_size": 1.,
           "text_colour" : "charcoal"})


#To the plot
plot(output, area, background, input, symbol, title)

#For documentation only
tofortran(ref, output, area, background, input, symbol, title)
tomv4(ref, symbol)
tohtml(ref, symbol)














