# $(addsuffix <prefix>, <name1 name2 ...>)
# add prefix as suffix in all names
# $(basename <names..>)
# fetch suffix in all names
# $(notdir <names..>)
# remove all documents path
# the 3 commands can generate obj names.

# $(patsubst <pattern>, <replacement>, <text>)
# if names in text match <pattern>, then replace it with <replacement>
# $(wildcard <pattern>)
# return all names match <pattern>
EXE = BRenderer
CXX = g++
DESTDIR = ./
IMGUI_DIR = ./imgui

# project cpps
SOURCES += $(wildcard *.cpp)
# imgui cpps
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_sdl.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp

OBJECTS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))

LIBS     = -lm -lGL -ldl `sdl2-config --libs`
CXXFLAGS = -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends
CXXFLAGS += -pg -Wall -Wformat
LDFLAGS = 

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

%.o:%.cpp
		$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/%.cpp
		$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/backends/%.cpp
		$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(DESTDIR)$(EXE)
		@echo Build complete for Linux

$(EXE): $(OBJECTS)
		$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
		rm -f $(EXE) $(OBJECTS) *.tga

