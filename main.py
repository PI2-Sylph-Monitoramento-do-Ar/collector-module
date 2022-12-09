import os
import random
import datetime as dt
from time import sleep

from typing import Tuple


class TemperatureSensor:
    DIRECTORY = './data/'
    FILE = 'temperatures.csv'

    def __init__(self) -> None:
        # if the directory doesn't exist, create it
        os.makedirs(self.DIRECTORY, exist_ok=True)

        # if file doesn't exist, create it
        if os.path.isfile(os.path.join(self.DIRECTORY, self.FILE)) is False:
            with open(os.path.join(self.DIRECTORY, self.FILE), 'w') as file:
                print('datetime;temperature', file=file)

    def get_temperature(self) -> Tuple[float, dt.datetime]:
        """
        Returns a random temperature between 15 and 35, and the datetime.
        Also saves the temperature in a file with the datetime as name.
        """
        temperature = float(random.randint(15, 35))
        datetime = dt.datetime.now()

        self.save_data(temperature, datetime)

        return temperature, datetime

    def save_data(self, temperature: float, datetime: dt.datetime):
        if isinstance(datetime, dt.datetime) is False:
            raise TypeError((
                'The datetime must be a dt.datetime! '
                f'Recieved: {type(datetime)}'
            ))

        if isinstance(temperature, float) is False:
            raise TypeError((
                'The temperature must be a float! '
                f'Recieved: {type(temperature)}'
            ))

        try:
            with open(os.path.join(self.DIRECTORY, self.FILE), 'a') as file:
                print(f'{datetime};{temperature}', file=file)
        except OSError as e:
            print(f'Error: {e}')

if __name__ == '__main__':

    temperature_sensor = TemperatureSensor()

    while True:
        temperature, datetime = temperature_sensor.get_temperature()

        print(f'Temperatura: {temperature}ºC')
        print('----------------------------------')

