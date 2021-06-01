#!/usr/bin/python

import os
from email.mime.text import MIMEText
import base64
from apiclient import errors

# all avaialble auto scopes: https://developers.google.com/gmail/api/auth/scopes
from googleAPIController import GoogleAPIController


class GmailController(GoogleAPIController):
    serviceName = "gmail"
    serviceVersion = "v1"

    def __init__(self, credentialName):
        GoogleAPIController.__init__(self, credentialName,
                                     self.serviceName,
                                     self.serviceVersion)

    @staticmethod
    def create_message_with_attachment(sender, to, subject, message_text, file):
        import mimetypes
        from email.mime.audio import MIMEAudio
        from email.mime.base import MIMEBase
        from email.mime.image import MIMEImage
        from email.mime.multipart import MIMEMultipart
        from email.mime.text import MIMEText
        """Create a message for an email.

        Args:
        sender: Email address of the sender.
        to: Email address of the receiver.
        subject: The subject of the email message.
        message_text: The text of the email message.
        file: The path to the file to be attached.

        Returns:
        An object containing a base64url encoded email object.
        """
        message = MIMEMultipart()
        message['to'] = to
        message['from'] = sender
        message['subject'] = subject

        msg = MIMEText(message_text)
        message.attach(msg)

        content_type, encoding = mimetypes.guess_type(file)

        if content_type is None or encoding is not None:
            content_type = 'application/octet-stream'
        main_type, sub_type = content_type.split('/', 1)
        if main_type == 'text':
            fp = open(file, 'r')
            msg = MIMEText(fp.read(), _subtype=sub_type)
            fp.close()
        elif main_type == 'image':
            fp = open(file, 'rb')
            msg = MIMEImage(fp.read(), _subtype=sub_type)
            fp.close()
        elif main_type == 'audio':
            fp = open(file, 'rb')
            msg = MIMEAudio(fp.read(), _subtype=sub_type)
            fp.close()
        else:
            fp = open(file, 'rb')
            msg = MIMEBase(main_type, sub_type)
            msg.set_payload(fp.read())
            fp.close()
        filename = os.path.basename(file)
        msg.add_header('Content-Disposition', 'attachment', filename=filename)
        message.attach(msg)

        return {'raw': base64.urlsafe_b64encode(message.as_bytes())
            .decode("utf-8")}

    @staticmethod
    def create_message(sender, to, subject, message_text):
        """Create a message for an email.

        Args:
        sender: Email address of the sender.
        to: Email address of the receiver.
        subject: The subject of the email message.
        message_text: The text of the email message.

        Returns:
        An object containing a base64url encoded email object.
        """
        message = MIMEText(message_text)
        message['to'] = to
        message['from'] = sender
        message['subject'] = subject
        return {'raw': base64.urlsafe_b64encode(message.as_bytes())
            .decode("utf-8")}

    # response structure: https://developers.google.com/gmail/api/v1/reference/users/messages#resource
    def sendMessage(self, message, user_id="me"):
        """Send an email message.

        Args:
          service: Authorized Gmail API service instance.
          user_id: User's email address. The special value "me"
          can be used to indicate the authenticated user.
          message: Message to be sent.

        Returns:
          Sent Message.
        """
        try:
            message = (self.service.users().messages()
                       .send(userId=user_id, body=message)
                       .execute())
            return message
        except errors.HttpError as error:
            print('An error occurred:', error)
            return None

    # response structure: https://developers.google.com/gmail/api/v1/reference/users/messages#resource
    def listMessagesMatchingQuery(self, user_id="me", query='', messageLimit=10):
        """List all Messages of the user's mailbox matching the query.

        Args:
          service: Authorized Gmail API service instance.
          user_id: User's email address. The special value "me"
          can be used to indicate the authenticated user.
          query: String used to filter messages returned.
          Eg.- 'from:user@some_domain.com' for Messages from a particular sender.

        Returns:
          List of Messages that match the criteria of the query. Note that the
          returned list contains Message IDs, you must use get with the
          appropriate ID to get the details of a Message.
        """
        try:
            response = self.service.users().messages() \
                .list(userId=user_id, q=query, maxResults=messageLimit).execute()
            messages = []
            if 'messages' in response:
                messages.extend(response['messages'])

            while 'nextPageToken' in response:
                if len(messages) >= messageLimit:
                    return messages
                page_token = response['nextPageToken']
                response = self.service.users().messages() \
                    .list(userId=user_id, q=query,
                          pageToken=page_token) \
                    .execute()
                messages.extend(response['messages'])

            return messages
        except errors.HttpError as error:
            print('An error occurred: ', error)

    def getMailDetail(self, mail_id, user_id="me", type="full"):
        response = self.service.users().messages() \
            .get(userId=user_id, id=mail_id) \
            .execute()
        return response

    def listMessagesWithLabels(self, user_id="me", label_ids=[], messageLimit=10):
        """List all Messages of the user's mailbox with label_ids applied.

          Args:
          service: Authorized Gmail API service instance.
          user_id: User's email address. The special value "me"
          can be used to indicate the authenticated user.
          label_ids: Only return Messages with these labelIds applied.

          Returns:
          List of Messages that have all required Labels applied. Note that the
          returned list contains Message IDs, you must use get with the
          appropriate id to get the details of a Message.
        """
        try:
            response = self.service.users().messages() \
                .list(userId=user_id, labelIds=label_ids,
                      maxResults=messageLimit) \
                .execute()
            messages = []
            if 'messages' in response:
                messages.extend(response['messages'])

            while 'nextPageToken' in response:
                page_token = response['nextPageToken']
                response = self.service.users().messages() \
                    .list(userId=user_id,
                          labelIds=label_ids,
                          pageToken=page_token) \
                    .execute()
                messages.extend(response['messages'])

            return messages
        except errors.HttpError as error:
            print('An error occurred: ', error)


if __name__ == "__main__":
    personalMailController = GmailController("<gmail-name>")
    dittoMailController = GmailController("<gmail-name>")
    from datetime import datetime
    import time

    number = 1
    for i in range(number):
        nowDate = datetime.now()
        nowStr = nowDate.strftime("%Y-%m-%d %H-%M-%S")
        message2Send = GmailController \
            .create_message(to="<to-address>",
                            sender="<from-address>",
                            subject=f"testMessage_{nowStr}",
                            message_text=f"welcome to automatic email sending {i}")
        preMsgList = dittoMailController.listMessagesMatchingQuery()
        sendResult = personalMailController.sendMessage(message2Send)
        sentTime = datetime.now()
        while True:
            latestMsgList = dittoMailController.listMessagesMatchingQuery()
            latestMsgId = latestMsgList[0]["id"]
            preMsgId = preMsgList[0]["id"]
            if latestMsgId == preMsgId:
                time.sleep(0.1)
                continue
            else:
                break
        receiveTime = datetime.now()
        timeCostDelta = receiveTime - sentTime
        timeCost = timeCostDelta.seconds + timeCostDelta.microseconds / float(1000000)
        print(f"time cost between sending and recieve is {timeCost} seconds")
