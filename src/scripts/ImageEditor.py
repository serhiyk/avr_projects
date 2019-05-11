import sys
import struct
from PyQt4 import QtGui, QtCore
from PyQt4.QtCore import QObject, SIGNAL


class MainWindow(QtGui.QWidget):
    def __init__(self):
        super(MainWindow, self).__init__()
        self.data = None
        self.initUI()

    def initUI(self):
        self.setWindowTitle('Image editor')

        self.MainLayout = QtGui.QVBoxLayout(self)

        self.ImageSizeGroupLay = QtGui.QHBoxLayout()
        self.ImageHeightEdit = QtGui.QLineEdit('0')
        self.ImageWidthEdit = QtGui.QLineEdit('0')
        self.ImageSizeButton = QtGui.QPushButton('Set', self)
        self.connect(self.ImageSizeButton, SIGNAL('clicked()'), self.ImageSizeButtonEvent)
        self.CodeLoadButton = QtGui.QPushButton('Load', self)
        self.connect(self.CodeLoadButton, SIGNAL('clicked()'), self.CodeLoadButtonEvent)
        self.InFileButton = QtGui.QPushButton("Open file", self)
        self.connect(self.InFileButton, SIGNAL('clicked()'), self.showSelectInFileDialog)
        self.ImageSizeGroupLay.addWidget(self.ImageHeightEdit)
        self.ImageSizeGroupLay.addWidget(self.ImageWidthEdit)
        self.ImageSizeGroupLay.addWidget(self.ImageSizeButton)
        self.ImageSizeGroupLay.addWidget(self.CodeLoadButton)
        self.ImageSizeGroupLay.addWidget(self.InFileButton)
        self.MainLayout.addLayout(self.ImageSizeGroupLay)

        self.ImageScene = QtGui.QGraphicsScene()
        self.ImageScene.mousePressEvent = self.ImageSceneEvent
        self.ImageView = QtGui.QGraphicsView(self.ImageScene)
        self.MainLayout.addWidget(self.ImageView)

        self.ImadgeRows = 0
        self.ImadgeColumns = 0
        self.PixelMatrix = [[0 for i in range(96)] for j in range(96)]
        self.ImageMatrix = []
        for i in range(96):
            self.ImageMatrix.append([])
            for j in range(96):
                rect = QtGui.QGraphicsRectItem(i * 10, j * 10, 10, 10)
                rect.setBrush(QtGui.QBrush(QtCore.Qt.white))
                rect.setPen(QtGui.QPen(QtCore.Qt.gray))
                self.ImageMatrix[i].append(rect)
                self.ImageScene.addItem(rect)

        self.CodeTextEdit = QtGui.QTextEdit()
        self.MainLayout.addWidget(self.CodeTextEdit)

        self.show()

    def showSelectInFileDialog(self):
        inFile = QtGui.QFileDialog.getOpenFileName(self, 'Open file')
        file = open(inFile, 'rb')
        data = file.read()
        file.close()
        if struct.unpack('>H', data[:2])[0] != 0x424D:
            print 'Error! Wrong file format.'
            return
        BitmapWidth = struct.unpack('<I', data[0x12:0x16])[0]
        BitmapHeight = struct.unpack('<I', data[0x16:0x1A])[0]
        BitsPerPixel = struct.unpack('<H', data[0x1C:0x1E])[0]
        if BitsPerPixel != 1:
            print 'Error! Wrong number of bits.'
            return
        BitmapAddress = struct.unpack('<I', data[0x0A:0x0E])[0]
        RowSize = ((BitmapWidth + 31) / 32) * 4
        print BitmapWidth
        print BitmapHeight
        print RowSize
        for y in range(BitmapHeight):
            for x in range(RowSize):
                byte = ord(data[BitmapAddress + y * RowSize + x])
                for bit in range(8):
                    if byte & (1 << (7 - bit)):
                        self.PixelMatrix[x * 8 + bit][BitmapHeight - 1 - y] = 0
                    else:
                        self.PixelMatrix[x * 8 + bit][BitmapHeight - 1 - y] = 1
        self.ImadgeRows = BitmapWidth
        self.ImadgeColumns = BitmapHeight
        self.UpdateImage(self.ImadgeRows, self.ImadgeColumns)
        self.UpdateCode()

    def ImageSizeButtonEvent(self):
        self.UpdateImage(int(self.ImageHeightEdit.text()), int(self.ImageWidthEdit.text()))

    def UpdateImage(self, rows, columns, data=None):
        self.ImadgeRows = rows
        self.ImadgeColumns = columns
        for x in range(96):
            for y in range(96):
                self.ImageMatrix[x][y].setBrush(QtGui.QBrush(QtCore.Qt.white))
                self.ImageMatrix[x][y].setPen(QtGui.QPen(QtCore.Qt.white))
        for x in range(rows):
            for y in range(columns):
                if self.PixelMatrix[x][y] == 0:
                    self.ImageMatrix[x][y].setBrush(QtGui.QBrush(QtCore.Qt.white))
                else:
                    self.ImageMatrix[x][y].setBrush(QtGui.QBrush(QtCore.Qt.black))
                self.ImageMatrix[x][y].setPen(QtGui.QPen(QtCore.Qt.gray))

    def ImageSceneEvent(self, event):
        if event.button() == QtCore.Qt.LeftButton:
            x = int(event.scenePos().x()) / 10
            y = int(event.scenePos().y()) / 10
            if self.PixelMatrix[x][y] == 0:
                self.ImageMatrix[x][y].setBrush(QtGui.QBrush(QtCore.Qt.black))
                self.PixelMatrix[x][y] = 1
            else:
                self.ImageMatrix[x][y].setBrush(QtGui.QBrush(QtCore.Qt.white))
                self.PixelMatrix[x][y] = 0
            self.UpdateCode()

    # def UpdateCode(self):
    #     code = ''
    #     for col in range(self.ImadgeColumns):
    #         x = 0
    #         for row in range(self.ImadgeRows):
    #             bit = row % 8
    #             if self.PixelMatrix[row][col] != 0:
    #                 x |= 1 << bit
    #             if bit == 7:
    #                 code += hex(x) + ','
    #                 x = 0
    #         code += '\n'
    #     self.CodeTextEdit.setText(code[:-2])

    def UpdateCode(self):
        code = ''
        for col in range(self.ImadgeColumns):
            x = 0
            for row in range(self.ImadgeRows):
                bit = row % 8
                if self.PixelMatrix[row][col] != 0:
                    x |= 1 << (7 - bit)
                if bit == 7:
                    code += hex(x) + ','
                    x = 0
            code += '\n'
        self.CodeTextEdit.setText(code[:-2])

    def CodeLoadButtonEvent(self):
        col = 0
        for line in str(self.CodeTextEdit.toPlainText()).replace(' ', '').split(',\n'):
            row = 0
            for hexByte in line.split(','):
                byte = int(hexByte, 16)
                for bit in range(8):
                    if byte & (1 << bit):
                        self.PixelMatrix[row * 8 + bit][col] = 1
                    else:
                        self.PixelMatrix[row * 8 + bit][col] = 0
                row += 1
            col += 1
        self.ImadgeRows = row * 8
        self.ImadgeColumns = col
        self.UpdateImage(self.ImadgeRows, self.ImadgeColumns)


if __name__ == '__main__':
    app = QtGui.QApplication(sys.argv)
    Main = MainWindow()
    sys.exit(app.exec_())
