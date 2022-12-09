import os
import random
import numbers
import logging
import datetime as dt
from typing import Tuple


logger = logging.getLogger(__name__)


i = 0
j = 6000


class Sensor:
    DIRECTORY = './data/'
    FILE = None
    VARIABLE_AND_UNIT = None

    def __init__(self) -> None:
        if self.FILE is None:
            raise NotImplementedError((
                'The FILE attribute must be set in the child class!'
            ))

        if self.VARIABLE_AND_UNIT is None:
            raise NotImplementedError((
                'The VARIABLE_AND_UNIT attribute must be set in the child class!'
            ))

        # if the directory doesn't exist, create it
        os.makedirs(self.DIRECTORY, exist_ok=True)

        # if file doesn't exist, create it
        if os.path.isfile(os.path.join(self.DIRECTORY, self.FILE)) is False:
            with open(os.path.join(self.DIRECTORY, self.FILE), 'w') as file:
                print(f'datetime;{self.VARIABLE_AND_UNIT}', file=file)

    def get_variable(self) -> float:
        raise NotImplementedError((
            'The get_variable method must be implemented in the child class!'
        ))

    def get_measure_datetime(self) -> dt.datetime:
        return dt.datetime.now()

    def save_data(self, value: float, datetime: dt.datetime):
        if isinstance(datetime, dt.datetime) is False:
            raise TypeError((
                'The datetime must be a dt.datetime! '
                f'Recieved: {type(datetime)}'
            ))

        # value must be a interger
        if isinstance(value, numbers.Number) is False:
            raise TypeError((
                'The value must be a number! '
                f'Recieved: {type(value)}'
            ))

        try:
            with open(os.path.join(self.DIRECTORY, self.FILE), 'a') as file:
                print(f'{datetime};{value}', file=file)
        except OSError as e:
            logger.error(f'Error writing to file: {e}')

    def measure(self) -> Tuple[float, dt.datetime]:
        value = self.get_variable()
        datetime = self.get_measure_datetime()
        self.save_data(value, datetime)
        return value, datetime


class TemperatureSensor(Sensor):
    DIRECTORY = './data/'
    FILE = 'temperatures.csv'
    VARIABLE_AND_UNIT = 'temperature(Celsius)'

    def get_variable(self) -> float:
        # TODO: Implement this method
        temperature = float(random.randint(15, 35))
        global i
        temperature = i
        i += 1
        return temperature


class CarbonMonoxideSensor(Sensor):
    DIRECTORY = './data/'
    FILE = 'carbon_monoxide.csv'
    VARIABLE_AND_UNIT = 'carbon_monoxide(ppm)'

    def get_variable(self) -> float:
        # TODO: Implement this method
        concentration = float(random.randint(15, 35))
        global j
        concentration = j
        j += 1
        return concentration