import pickle
from pathlib import Path
from googleapiclient.discovery import build
from google_auth_oauthlib.flow import InstalledAppFlow
from google.auth.transport.requests import Request

STORAGE_PATH = Path(Path.home(), 'Research', 'capture', '.credentials')
TOKEN_NAME = 'token.pickle'

SCOPES = [
    "https://www.googleapis.com/auth/gmail.readonly",
    "https://www.googleapis.com/auth/gmail.modify",
    "https://www.googleapis.com/auth/drive",
]



class GoogleAPIController(object):
    def __init__(self, credentialName, serviceName, serviceVersion):
        self.credentialName = credentialName
        self.flags = None
        self.storage_dir = STORAGE_PATH
        if not self.storage_dir.exists():
            Path.mkdir(self.storage_dir)
        self.credentialPath = Path(self.storage_dir, f"{self.credentialName}.json")
        self.credentials = self.initCredentials()
        print('Storing credentials to ' + str(self.credentialPath))

        self.service = build(serviceName,
                             serviceVersion,
                             credentials=self.credentials)

    def initCredentials(self):
        """Gets valid user credentials from storage.

        If nothing has been stored, or if the stored credentials are invalid,
        the OAuth2 flow is completed to obtain the new credentials.

        Returns:
            Credentials, the obtained credential.
        """
        creds = None
        token_store_path = Path(self.storage_dir, self.credentialName + TOKEN_NAME)
        if token_store_path.exists():
            with open(token_store_path, 'rb') as token:
                creds = pickle.load(token)
        if not creds or not creds.valid:
            if creds and creds.expired and creds.refresh_token:
                creds.refresh(Request())
            else:
                flow = InstalledAppFlow.from_client_secrets_file(
                    self.credentialPath, SCOPES)
                creds = flow.run_local_server(port=0)
                # Save the credentials for the next run
            with open(token_store_path, 'wb') as token:
                pickle.dump(creds, token)
        return creds
