TARGET	= calib
IMGUI	= libs/ImGui
SOURCES	= $(wildcard *.cpp) $(wildcard $(IMGUI)/imgui*.cpp) $(IMGUI)/nfd_gtk.cpp
HEADERS	= $(wildcard *.h)
OBJECTS	= $(patsubst %.cpp,%.o,$(SOURCES))
CXXFLAGS	= --std=c++17 -pthread -g -Wall -DDEBUG -DX11 -DPROJECT_NAME=\"$(TARGET)\" \
	-Ilibs/include `pkg-config opencv4 --cflags` `pkg-config gtk+-3.0 --cflags` `pkg-config glfw3 --cflags` \
	-I$(IMGUI)
LDLIBS	= -ldl -lGL `pkg-config opencv4 --libs` `pkg-config gtk+-3.0 --libs` `pkg-config glfw3 --libs`

.PHONY: clean

$(TARGET): $(OBJECTS)
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(TARGET).dep: $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -MM $(SOURCES) > $@

clean:
	-$(RM) $(TARGET) *.o *~ .*~ *.bak *.dep imgui.ini a.out core $(IMGUI)/*.o

-include $(TARGET).dep
