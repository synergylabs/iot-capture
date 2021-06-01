import sys
import argparse
import time
import datetime
import uuid

import wemo
from googleSheetController import GoogleSheetController
from gmailController import GmailController

datetimeFormat = "%Y-%m-%d %H-%M-%S.%f"


# if my wemo switch is activated, add line to spreadsheet
# curl -X POST -H "Content-Type: application/json" -d '{"value1":"a"}' \
def test_wemo_google_sheet_recipe(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument("resultFile")
    parser.add_argument("-iterNum", default=30, type=int)
    parser.add_argument("-executionInterval", default=20, type=float)
    parser.add_argument("-googleQueryInterval", default=0.25, type=float)
    parser.add_argument("-googleMaxRetry", default=10, type=int)
    parser.add_argument("-wemoport", type=int, default=10085)
    parser.add_argument("-wemoip", default="0.0.0.0", type=str)
    options = parser.parse_args(argv)

    bind = "{}:{}".format(options.wemoip, options.wemoport)
    wemo_controller = wemo.WemoController(bind=bind)
    switch = wemo_controller.discoverSwitch()
    if switch is None:
        print("error to locate the switch")
        sys.exit(1)
    else:
        print("switch discovered")

    spreadsheet_id = "<spreadsheet-id>"
    sheet_name = "Sheet1"
    sheet_controller = GoogleSheetController()
    spreadsheet = sheet_controller.getSpreadSheet(spreadsheet_id)
    print("got spreadsheet: ", sheet_controller.getSpreadSheet(spreadsheet_id))
    retrieved_sheet_name = spreadsheet["sheets"][0]["properties"]["title"]
    print("title of first sheet is ", spreadsheet["sheets"][0]["properties"]["title"])
    if retrieved_sheet_name != sheet_name:
        print(f"sheet name doesn't match, use retrieved one: preconfigured one: {sheet_name}, "
              f"retrieved one {retrieved_sheet_name}")
        sheet_name = retrieved_sheet_name
    current_row_list = sheet_controller.getRowList(spreadsheet_id, range=sheet_name)
    print("current row list", current_row_list)
    if current_row_list is None:
        current_row_list = []

    result_stat_list = []
    wemo_controller.turnoffSwitch()
    result_fd = open(options.resultFile, "a")
    # test recipe: when wemo switch is truned on, write a log to the google spreadsheet
    for index in range(options.iterNum):
        time.sleep(options.executionInterval)
        test_uuid = uuid.uuid4()

        current_row_list = sheet_controller.getRowList(spreadsheet_id, range=sheet_name)
        print("current row list", current_row_list)
        if current_row_list is None:
            current_row_list = []

        local_start_time = datetime.datetime.now()
        wemo_controller.turnonSwitch()
        trigger_generated_time = datetime.datetime.now()
        print("switch turned on")

        success = False
        attempts = 0
        while (not success) and attempts < options.googleMaxRetry:
            attempts += 1
            latest_row_list = sheet_controller.getRowList(spreadsheet_id,
                                                          range=sheet_name)
            if latest_row_list is not None and len(latest_row_list) == len(current_row_list) + 1:
                print(latest_row_list[-1])
                # current_row_list = latest_row_list
                success = True
            if latest_row_list is None:
                print("error when requesting latest row list")
            time.sleep(options.googleQueryInterval)
        end_time = datetime.datetime.now()

        if success:
            write_result(result_fd=result_fd,
                         result_stat_list=result_stat_list,
                         local_start_time=local_start_time,
                         trigger_generated_time=trigger_generated_time,
                         end_time=end_time,
                         eval_name='wemo_sheet',
                         index=index,
                         test_uuid=test_uuid)
        else:
            index -= 1

        wemo_controller.turnoffSwitch()
    result_fd.close()


def write_result(result_fd, result_stat_list, local_start_time, trigger_generated_time, end_time, eval_name, index,
                 test_uuid=None):
    if end_time is None:
        print_stat(result_fd, result_stat_list,
                   f"testUuid: {test_uuid}->failed for iter {index}")
        return

    if test_uuid is None:
        test_uuid = uuid.uuid4()

    time_diff = end_time - local_start_time
    time_cost = time_diff.seconds + time_diff.microseconds / float(1_000_000)
    print_stat(result_fd, result_stat_list,
               f"testUuid: {test_uuid}->triggerGeneratedTime: "
               f"{trigger_generated_time.strftime(datetimeFormat)}"
               f"->{eval_name}")
    print_stat(result_fd, result_stat_list,
               f"testUuid: {test_uuid}->localActionStartTime: "
               f"{local_start_time.strftime(datetimeFormat)}"
               f"->{eval_name}")
    print_stat(result_fd, result_stat_list,
               f"testUuid: {test_uuid}->actionResultObservedTime: "
               f"{end_time.strftime(datetimeFormat)}"
               f"->{eval_name}")
    print_stat(result_fd, result_stat_list,
               f"testUuid: {test_uuid}->time cost for iter {index} "
               f"is {time_cost} seconds")


# when any new email arrives in gmail, blink lights.
def test_gmail_hue_recipe(argv):
    '''
    test the following recipe:
    If you send an email to IFTTT endpoint, then a trigger sends a command to turn on the switch.
    '''
    parser = argparse.ArgumentParser()
    parser.add_argument("resultFile")
    parser.add_argument("-iterNum", default=1, type=int)
    parser.add_argument("-executionInterval", default=10, type=float)
    parser.add_argument("-gmailSenderName", default="gmail-ditto", type=str)
    options = parser.parse_args(argv)

    wemo_notification_controller = wemo.WemoNotificationController()
    gmail_sender_controller = GmailController(options.gmailSenderName)
    result_stat_list = []
    result_fd = open(options.resultFile, "a")

    for index in range(options.iterNum):
        time.sleep(options.executionInterval)
        local_start_time = datetime.datetime.now()
        now_str = local_start_time.strftime("%Y-%m-%d %H-%M-%S")
        subject = "ifttt test at {}".format(now_str)
        message_body = subject
        message = GmailController.create_message(
            sender="<from-address>",
            to="trigger@applet.ifttt.com",
            subject=subject,
            message_text=message_body)
        gmail_sender_controller.sendMessage(message)
        trigger_generated_time = datetime.datetime.now()

        wemo_notification_controller.register_start_time(trigger_generated_time)
        wemo_notification_controller.wait_for_notification()
        end_time = wemo_notification_controller.get_notification_time()

        write_result(result_fd=result_fd,
                     result_stat_list=result_stat_list,
                     local_start_time=local_start_time,
                     trigger_generated_time=trigger_generated_time,
                     end_time=end_time,
                     eval_name='gmail_hue',
                     index=index)
    result_fd.close()


def test_wemo_hue_recipe(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument("resultFile")
    parser.add_argument("-iterNum", default=5, type=int)
    parser.add_argument("-executionInterval", default=10, type=float)
    parser.add_argument("-wemoport", type=int, default=10085)
    parser.add_argument("-wemoip", default="0.0.0.0", type=str)
    options = parser.parse_args(argv)

    action_controller = wemo.WemoNotificationController()
    bind = "{}:{}".format(options.wemoip, options.wemoport)
    trigger_controller = wemo.WemoController(bind=bind)
    switch = trigger_controller.discoverSwitch()
    if switch is None:
        print("error to locate the switch")
        sys.exit(1)
    else:
        print("switch discovered")

    result_stat_list = []
    result_fd = open(options.resultFile, "a")
    # test recipe: when wemo switch is truned on, turn on lights in living room
    for index in range(options.iterNum):
        print(f"start test iteration {index}")
        trigger_controller.turnoffSwitch()
        time.sleep(options.executionInterval)

        local_start_time = datetime.datetime.now()

        # generate trigger event: turn on switch
        trigger_controller.turnonSwitch()
        trigger_generated_time = datetime.datetime.now()
        print("switch turned one")

        action_controller.register_start_time(trigger_generated_time)
        action_controller.wait_for_notification()
        end_time = action_controller.get_notification_time()

        write_result(result_fd=result_fd,
                     result_stat_list=result_stat_list,
                     local_start_time=local_start_time,
                     trigger_generated_time=trigger_generated_time,
                     end_time=end_time,
                     eval_name="wemo_hue",
                     index=index)
    result_fd.close()


def print_stat(result_fd, result_stat_list, stat_str):
    print(stat_str)
    result_stat_list.append(stat_str)
    result_fd.write(stat_str + "\n")
    result_fd.flush()


recipeTypeDict = {
    "wemo_hue": test_wemo_hue_recipe,
    "gmail_hue": test_gmail_hue_recipe,
    "wemo_sheet": test_wemo_google_sheet_recipe,
}

if __name__ == "__main__":
    recipeName = sys.argv[1]
    print(recipeName)
    if recipeName not in recipeTypeDict:
        print("please provide recipeType from this list: ", recipeTypeDict.keys())
        sys.exit(1)
    recipeFunc = recipeTypeDict[recipeName]
    recipeFunc(sys.argv[2:])
