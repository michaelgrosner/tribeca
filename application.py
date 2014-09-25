import functools
import gevent
import logbook
import time
import requests
import gevent.monkey
from socketIO_client import SocketIO

gevent.monkey.patch_all()


class MarketUpdate(object):
    def __init__(self, bid_sz, bid_px, ask_px, ask_sz, time):
        self.time = time
        self.ask_sz = ask_sz
        self.ask_px = ask_px
        self.bid_px = bid_px
        self.bid_sz = bid_sz

    def __eq__(self, other):
        """
        :type other: MarketUpdate
        """
        return self.ask_sz == other.ask_sz and self.ask_px == other.ask_px and \
               self.bid_px == other.bid_px and self.bid_sz == other.bid_sz

    def __str__(self):
        return self.__repr__()

    def __repr__(self):
        return "{0:.2f} {1:.2f}-{2:.2f} {3:.2f}".format(self.bid_sz, self.bid_px, self.ask_px, self.ask_sz)


class Coinsetter(object):
    def __init__(self, callback):
        self._callback = callback
        self._socket = SocketIO('https://plug.coinsetter.com', 3000,
                                wait_for_connection=True,
                                transports=('websocket', ))
        self._socket.on('connect', lambda: gevent.spawn(self._handle_connect))
        self._socket.on('ticker', lambda x: gevent.spawn(lambda: self._handle_depth(x)))
        self._is_alive = True

    def start(self):
        self._socket.wait()

    def stop(self):
        self._is_alive = False

    def _handle_depth(self, depth):
        self._callback(MarketUpdate(depth[u'bid'][u'size'], depth[u'bid'][u'price'],
                                    depth[u'ask'][u'price'], depth[u'ask'][u'size'], time.time()))

    def _handle_connect(self):
        log.info("Connected")
        self._socket.emit('ticker room', '')


def _px(x):
    return float(x[0])


def _sz(x):
    return float(x[1])


class HitBtc(object):
    def __init__(self, callback):
        self._callback = callback
        self._is_started = True

    def start(self):
        while self._is_started:
            orderbook = requests.get("http://api.hitbtc.com/api/1/public/BTCUSD/orderbook").json()
            self._callback(MarketUpdate(_sz(orderbook['bids'][0]), _px(orderbook['bids'][0]),
                                        _px(orderbook['asks'][0]), _sz(orderbook['asks'][0]),
                                        time.time()))
            gevent.sleep(10)

    def stop(self):
        self._is_started = False


class Coalescer(object):
    def __init__(self, callback):
        self._callback = callback
        self._last = None

    def combine(self, new):
        if not self._last or not (self._last == new):
            self._last = new
            self._callback(new)


log = logbook.Logger(__name__)

if __name__ == "__main__":
    def _handle(ex, mu):
        log.info("{} dt={} {}", ex, time.time() - mu.time, mu)

    cs_coalescer = Coalescer(functools.partial(_handle, "CS"))
    coinsetter_market_data = Coinsetter(cs_coalescer.combine)

    hb_coalescer = Coalescer(functools.partial(_handle, "HB"))
    hitbtc_market_data = HitBtc(hb_coalescer.combine)

    tasks = [coinsetter_market_data, hitbtc_market_data]

    try:
        gevent.joinall(map(gevent.spawn, (getattr(t, "start") for t in tasks)), raise_error=True)
    except Exception, e:
        for x in tasks:
            x.stop()
        log.exception("Unhandled exception during main loop", e)
        raise