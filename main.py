import signal
import logging
from time import sleep

from sensors import TemperatureSensor, CarbonMonoxideSensor


logger = logging.getLogger(__name__)


if __name__ == '__main__':
    IS_SHUTTING_DOWN = False

    def graceful_exit(*args, **kwargs):
        """
        This function is called when the worker receives a stop signal.
        This function signals that the worker should stop as soon as possible.
        """
        global IS_SHUTTING_DOWN
        IS_SHUTTING_DOWN = True

    signal.signal(signal.SIGINT, graceful_exit)
    signal.signal(signal.SIGTERM, graceful_exit)

    temperature_sensor = TemperatureSensor()
    # co_sensor = CarbonMonoxideSensor()

    while not IS_SHUTTING_DOWN:
        temperature, datetime = temperature_sensor.measure()
        # co_concentration, datetime = co_sensor.measure()

        print(f'Temperatura: {temperature}ºC')
        # print(f'Concentração de CO: {co_concentration}ppm')
        print('----------------------------------')
