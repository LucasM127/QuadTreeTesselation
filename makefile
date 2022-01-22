CC = g++ -g -Wall
LIBS = -lsfml-system -lsfml-window -lsfml-graphics
SFML_DIR = /home/lucas/Headers/SFML-2.5.1
INC = -I $(SFML_DIR)/include
LINK = -L $(SFML_DIR)/lib $(LIBS) -Wl,-rpath=$(SFML_DIR)/lib
OF = Obj
LF = Lib
TF = Tests
OBJS = $(OF)/QuadTreeRayCast.o $(OF)/QuadTreeTesselator.o $(OF)/TriangulateCellNode.o $(OF)/QuadTreeTriangulate.o $(OF)/QuadTreeRemoveDuplicates.o $(OF)/DQT.o

all: $(OF) $(TF) $(LF)
	make $(TF)/sceneTest
	make $(TF)/neighbourTest
	make $(TF)/duplicatePointsTest
	make $(TF)/lineClipTest

$(TF):
	[ -d $@ ] || mkdir $@

$(LF):
	[ -d $@ ] || mkdir $@

$(OF):
	[ -d $@ ] || mkdir $@

$(LF)/TessLib.a: $(OBJS) $(LF)
	ar -rvs $@ $(OBJS)

$(TF)/sceneTest: $(OF)/SceneTest.o $(LF)/TessLib.a
	$(CC) $(OF)/SceneTest.o TessLib.a -o $@ $(LINK)

$(TF)/neighbourTest: $(OF)/NeighbourTest.o $(OF)/DQT.o
	$(CC) $(OF)/NeighbourTest.o $(OF)/DQT.o -o $@ $(LINK)

$(TF)/lineClipTest: $(OF)/LineClipTest.o
	$(CC) $(OF)/LineClipTest.o -o $@ $(LINK)

$(TF)/duplicatePointsTest: $(OF)/PointGenTest.o $(LF)/TessLib.a
	$(CC) $(OF)/PointGenTest.o $(LF)/TessLib.a -o $@ $(LINK)

$(OF)/QuadTreeRemoveDuplicates.o: QuadTreeRemoveDuplicates.cpp
	$(CC) $< -c -o $@ $(INC)

$(OF)/QuadTreeRayCast.o: QuadTreeRayCast.cpp
	$(CC) $< -c -o $@ $(INC)

$(OF)/QuadTreeTriangulate.o: QuadTreeTriangulate.cpp
	$(CC) $< -c -o $@ $(INC)

$(OF)/QuadTreeTesselator.o: QuadTreeTesselator.cpp
	$(CC) $< -c -o $@ $(INC)

$(OF)/TriangulateCellNode.o: TriangulateCellNode.cpp
	$(CC) $< -c -o $@ $(INC)

$(OF)/DQT.o: DQT.cpp DQT.hpp
	$(CC) $< -c -o $@

$(OF)/SceneTest.o: SceneTest.cpp
	$(CC) $< -c -o $@ $(INC)

$(OF)/NeighbourTest.o: NeighbourTest.cpp $(OF)/DQT.o
	$(CC) $< -c -o $@ $(INC)

$(OF)/LineClipTest.o: LineClipTest.cpp
	$(CC) $< -c -o $@ $(INC)

$(OF)/PointGenTest.o: PointGenTest.cpp
	$(CC) $< -c -o $@ $(INC)

clean:
	rm $(OF)/*.o TessLib.a

.Phony: clean