CC = g++ -O2 #-g -Wall
LIBS = -lsfml-system -lsfml-window -lsfml-graphics
SFML_DIR = /home/lucas/Headers/SFML-2.5.1
INC = -I $(SFML_DIR)/include
LINK = -L $(SFML_DIR)/lib $(LIBS) -Wl,-rpath=$(SFML_DIR)/lib
OF = Obj
LF = Lib
TF = Tests
OBJS = $(OF)/QuadTreeRayCast.o $(OF)/QuadTreeTesselator.o $(OF)/TriangulateCellNode.o $(OF)/QuadTreeTriangulate.o \
		$(OF)/QuadTreeRemoveDuplicates.o $(OF)/DQT.o $(OF)/Interval.o $(OF)/Consts.o

#TODO: RESTRUCTURE tests code out of here
#TODO: fix dependencies of make all tests

all: $(OF) $(TF) $(LF)
	make $(TF)/sceneTest
	make $(TF)/neighbourTest
	make $(TF)/duplicatePointsTest
	make $(TF)/lineClipTest
	make $(TF)/intervalTest
	make $(TF)/cellTriTest
	make $(TF)/pointyTest

$(TF):
	[ -d $@ ] || mkdir $@

$(LF):
	[ -d $@ ] || mkdir $@

$(OF):
	[ -d $@ ] || mkdir $@

$(LF)/TessLib.a: $(OBJS) $(LF)
	ar -rvs $@ $(OBJS)

$(TF)/sceneTest: $(OF)/SceneTest.o $(LF)/TessLib.a
	$(CC) $(OF)/SceneTest.o $(LF)/TessLib.a -o $@ $(LINK)

$(TF)/neighbourTest: $(OF)/NeighbourTest.o $(OF)/DQT.o $(OF)/Consts.o
	$(CC) $(OF)/NeighbourTest.o $(OF)/DQT.o $(OF)/Consts.o -o $@ $(LINK)

$(TF)/lineClipTest: $(OF)/LineClipTest.o
	$(CC) $(OF)/LineClipTest.o -o $@ $(LINK)

$(TF)/duplicatePointsTest: $(OF)/PointGenTest.o $(LF)/TessLib.a
	$(CC) $(OF)/PointGenTest.o $(LF)/TessLib.a -o $@ $(LINK)

$(TF)/intervalTest: $(OF)/IntervalTest.o $(OF)/Interval.o
	$(CC) $(OF)/IntervalTest.o $(OF)/Interval.o -o $@

$(TF)/cellTriTest: $(OF)/TriangulateCellNode.o $(OF)/TriangulateCellNodeTest.o $(OF)/Interval.o $(OF)/Consts.o
	$(CC) $(OF)/TriangulateCellNode.o $(OF)/TriangulateCellNodeTest.o $(OF)/Interval.o $(OF)/Consts.o -o $@

$(TF)/pointyTest: $(OF)/pointyTest.o $(OF)/SFMLCamera.o $(LF)/TessLib.a
	$(CC) $(OF)/pointyTest.o $(OF)/SFMLCamera.o $(LF)/TessLib.a -o $@ -lGL $(LINK)

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

$(OF)/Consts.o: Consts.cpp Consts.hpp
	$(CC) $< -c -o $@

$(OF)/SceneTest.o: SceneTest.cpp
	$(CC) $< -c -o $@ $(INC)

$(OF)/NeighbourTest.o: NeighbourTest.cpp
	$(CC) $< -c -o $@ $(INC)

$(OF)/LineClipTest.o: LineClipTest.cpp
	$(CC) $< -c -o $@ $(INC)

$(OF)/PointGenTest.o: PointGenTest.cpp
	$(CC) $< -c -o $@ $(INC)

$(OF)/Interval.o: Interval.cpp Interval.hpp
	$(CC) $< -c -o $@

$(OF)/IntervalTest.o: IntervalTest.cpp $(OF)/Interval.o
	$(CC) $< -c -o $@

$(OF)/TriangulateCellNodeTest.o: TriangulateCellNodeTest.cpp
	$(CC) $< -c -o $@ $(INC)

$(OF)/pointyTest.o: pointyTest.cpp
	$(CC) $< -c -o $@ $(INC)

$(OF)/SFMLCamera.o: SFMLCamera.cpp SFMLCamera.hpp
	$(CC) $< -c -o $@ $(INC)

clean:
	rm $(OF)/*.o $(LF)/TessLib.a

.Phony: clean