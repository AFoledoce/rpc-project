from enum import Enum
from typing import Mapping, Optional

from rpcclient.exceptions import MissingLibraryError, PermissionDeniedError
from rpcclient.structs.consts import RTLD_NOW


class CLAuthorizationStatus(Enum):
    kCLAuthorizationStatusNotDetermined = 0
    kCLAuthorizationStatusRestricted = 1
    kCLAuthorizationStatusDenied = 2
    kCLAuthorizationStatusAuthorizedAlways = 3
    kCLAuthorizationStatusAuthorizedWhenInUse = 4
    kCLAuthorizationStatusAuthorized = kCLAuthorizationStatusAuthorizedAlways

    @classmethod
    def from_value(cls, value: int):
        for i in cls:
            if i.value == value:
                return i


class Location:
    """
    Wrapper to CLLocationManager
    https://developer.apple.com/documentation/corelocation/cllocationmanager?language=objc
    """

    def __init__(self, client):
        self._client = client

        self._load_location_library()
        self._CLLocationManager = self._client.symbols.objc_getClass('CLLocationManager')
        self._location_manager = self._CLLocationManager.objc_call('sharedManager')

    def _load_location_library(self):
        options = [
            # macOS
            '/System/Library/Frameworks/CoreLocation.framework/Versions/A/CoreLocation',
            # iOS
            '/System/Library/Frameworks/CoreLocation.framework/CoreLocation'
        ]
        for option in options:
            if self._client.dlopen(option, RTLD_NOW):
                return
        raise MissingLibraryError('CoreLocation library isn\'t available')

    @property
    def location_services_enabled(self) -> bool:
        return bool(self._location_manager.objc_call('locationServicesEnabled'))

    @location_services_enabled.setter
    def location_services_enabled(self, value: bool):
        self._CLLocationManager.objc_call('setLocationServicesEnabled:', value)

    @property
    def authorization_status(self) -> CLAuthorizationStatus:
        return CLAuthorizationStatus.from_value(self._location_manager.objc_call('authorizationStatus'))

    @property
    def last_sample(self) -> Optional[Mapping]:
        return self._location_manager.objc_call('location').objc_call('jsonObject').py

    def start_updating_location(self):
        if self.authorization_status.value < CLAuthorizationStatus.kCLAuthorizationStatusAuthorizedAlways.value:
            raise PermissionDeniedError()
        self._location_manager.objc_call('startUpdatingLocation')

    def stop_updating_location(self):
        self._location_manager.objc_call('stopUpdatingLocation')

    def request_oneshot_location(self):
        """ Requests the one-time delivery of the user’s current location. """
        if self.authorization_status.value < CLAuthorizationStatus.kCLAuthorizationStatusAuthorizedAlways.value:
            raise PermissionDeniedError()
        self._location_manager.objc_call('requestLocation')
