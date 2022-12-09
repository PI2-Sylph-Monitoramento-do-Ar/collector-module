import os
import logging
# import requests
# from typing import List
from time import sleep

# Set logging level
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)


class Uploader:
    """
    Class that monitors csv files that are inside a directory, and uploads the
    data contained in such files when there is an internet connection.

    For each .csv file this class creates a file with the same name but with
    a .seek extension, where it stores the next line of the file that must
    be processed.
    """

    def __init__(self, data_directory: str, sleep_time: int = 5):
        if os.path.isdir(data_directory) is False:
            raise ValueError('The directory does not exist!')

        self.data_dir = data_directory
        self.sleep_time = sleep_time

    @staticmethod
    def get_or_create_file_position(filepath: str):
        """
        Returns the file position of the file, if it does not exist, it creates
        """
        if isinstance(filepath, str) is False:
            raise TypeError('The filepath must be a string!')

        if filepath.endswith('.csv') is False:
            raise TypeError('The filepath must end with `.csv`!')

        filepath = filepath[:-4] + '.seek'

        # Create the file if it does not exist.
        if os.path.isfile(filepath) is False:
            logger.info(f'Creating file {filepath}')
            with open(filepath, 'w') as file:
                print('0', file=file)

        return filepath

    @staticmethod
    def get_last_line(filename):
        """
        Returns the last line of the file.
        TODO: There are more efficient ways to do this.
        """
        with open(filename, 'rb') as file:
            for line in file:
                pass
            return int(line)

    @staticmethod
    def extract_data(raw_line: str) -> dict:
        datetime, value = raw_line.strip().split(';')
        return {'datetime': datetime, 'value': value}

    def upload_data(self, data: dict):
        """
        Upload the data to the server
        """
        print(data)

    def process_file(self, filepath: str) -> bool:
        """
        Process the new data contained in the file
        """
        file_position: str = self.get_or_create_file_position(filepath)
        last_position: int = self.get_last_line(file_position)

        something_new = False

        with open(filepath, 'r') as file, open(file_position, 'a') as file_seek:
            # Jump to next position to be processed
            file.seek(last_position)

            line = file.readline()
            while line:
                something_new = True
                extract_data = self.extract_data(line)
                self.upload_data(extract_data)
                print(file.tell(), file=file_seek)
                line = file.readline()

        return something_new


    def watch(self):
        while True:
            something_new = False

            logger.info('Searching for new data in monitored files')
            for file in os.listdir(self.data_dir):
                if file.endswith('.csv'):
                    logger.info("Searching for new data in file '%s'", file)

                    filepath = f'{self.data_dir}/{file}'
                    something_new |= self.process_file(filepath)

                    if something_new is True:
                        logger.info('New data found in file %s', file)
                    else:
                        logger.info('No new data found in file %s', file)

            # If all files in the directory are processed and there is no
            # new data, it sleeps for 30 seconds.
            if something_new is False:
                logger.info('No new data found in any of the monitored files, sleeping for %s seconds', self.sleep_time)
                sleep(self.sleep_time)

if __name__ == '__main__':
    uploader = Uploader('./data')

    uploader.watch()




# def process(line: str):
#     line = line.strip()
#     datetime, temperature = line.split(';')
#     data = {'datetime': datetime, 'temperature': temperature}
#     print(data)

# if __name__ == '__main__':
#     while True:
#         for file_data in os.listdir(DATA_DIRECTORY):
#             if file_data.endswith('.csv'):
#                 file_position: str = get_file_position(file_data)
#                 last_position: int = get_last_line(file_position)

#                 filepath = f'{DATA_DIRECTORY}/{file_data}'
#                 with open(filepath, 'r') as file, open(file_position, 'a') as file_seek:
#                     file.seek(last_position)

#                     line = file.readline()

#                     while line:
#                         process(line)
#                         next_position = file.tell()
#                         print(next_position, file=file_seek)
#                         line = file.readline()







