from googleAPIController import GoogleAPIController


class GoogleSheetController(GoogleAPIController):
    serviceName = "sheets"
    serviceVersion = "v4"

    def __init__(self):
        GoogleAPIController.__init__(self, "googleDriveController",
                                     self.serviceName,
                                     self.serviceVersion)
        self.testSpreadsheetId = ""
        self.testSheetTitle = ""

    def getSpreadSheet(self, spreadsheetId):
        return self.service.spreadsheets().get(spreadsheetId=spreadsheetId).execute()

    def createFile(self, name, initSheetName):
        body = {
            "properties": {
                "title": name,
            },
            "sheets": [
                {
                    "properties": {
                        "title": initSheetName,
                    }
                }
            ],
        }
        response = self.service.spreadsheets().create(body=body).execute()
        return response

    def listFileRevisions(self, fileId):
        revisionOp = self.service.revisions()
        revisionList = revisionOp.list(fileId=fileId).execute()
        return revisionList

    def appendFile(self, spreadsheetId, range, valueList):
        body = {
            "values": valueList,
        }
        response = self.service.spreadsheets().values().append(spreadsheetId=spreadsheetId, range=range, body=body,
                                                               insertDataOption="INSERT_ROWS",
                                                               valueInputOption="RAW").execute()
        return response

    def clearFileContent(self, spreadsheetId, range):
        response = self.service.spreadsheets().values().clear(spreadsheetId=spreadsheetId, range=range).execute()

    def getFileContent(self, spreadsheetId, range):
        response = self.service.spreadsheets().values().get(spreadsheetId=spreadsheetId, range=range).execute()
        return response

    def getRowList(self, spreadsheetId, range):
        try:
            response = self.service.spreadsheets().values().get(spreadsheetId=spreadsheetId, range=range).execute()

            if "values" in response:
                return response["values"]
            return []
        except Exception as e:
            return None

    def fileExists(self, filePath):
        pass

    def rmFile(self, filePath):
        pass

    def mvFile(self, srcPath, dstPath):  # include files and directories
        pass

    def ls(self, parentId=None):
        pageToken = None
        overallFileList = []
        if parentId is None:
            q = "mimeType='application/vnd.google-apps.folder'"
        else:
            q = "mimeType='application/vnd.google-apps.folder' and '{}' in parents".format(parentId)
        while True:
            fileList = self.service.files().list(q=q, orderBy="createdTime", spaces="drive",
                                                 fields='nextPageToken, files(id, name)', pageToken=pageToken).execute()
            overallFileList.extend(fileList.get("files", []))
            pageToken = fileList.get("nextPageToken", None)
            if pageToken is None:
                break
        return overallFileList

    def getFileInfo(self, filePath):
        pass

    def setFileInfo(self, filePath, fileInfo):
        pass


if __name__ == "__main__":
    spreadsheetId = "<spreadsheet-id>"
    sheetController = GoogleSheetController()
    spreadsheet = sheetController.getSpreadSheet(spreadsheetId)
    print("got spreadsheet: ", sheetController.getSpreadSheet(spreadsheetId))
    print("title of first sheet is ", spreadsheet["sheets"][0]["properties"]["title"])
    valueList = sheetController.getRowList(spreadsheetId, range="Sheet1")
    print(valueList)
