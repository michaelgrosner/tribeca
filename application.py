import functools
import gevent
from gevent.event import AsyncResult, Event
from gevent.queue import Channel
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
    def __init__(self):
        self._socket = SocketIO('https://plug.coinsetter.com', 3000,
                                wait_for_connection=True,
                                transports=('websocket', ))

        self._depth_evt = Channel()
        self._socket.on('connect', self._handle_connect)
        self._socket.on('depth', self._handle_depth)
        self._is_alive = True

    def start(self):
        self._socket.wait()

    def stop(self):
        self._is_alive = False

    def _handle_depth(self, depth):
        def _level(l):
            return MarketUpdate(depth[l][u'bid'][u'size'], depth[l][u'bid'][u'price'],
                                depth[l][u'ask'][u'price'], depth[l][u'ask'][u'size'], time.time())

        self._depth_evt.put(("CS", _level(0), _level(1)))

    def depth_stream(self):
        evt = AsyncResult()
        gevent.spawn(self._depth_evt.get).link(evt.set)
        return evt

    def _handle_connect(self):
        log.info("Connected")
        self._socket.emit('depth room', '')


def _px(x):
    return float(x[0])


def _sz(x):
    return float(x[1])


class HitBtc(object):
    def __init__(self):
        self._is_started = True
        self._depth_evt = Channel()

    def start(self):
        while self._is_started:
            def _level(l):
                return MarketUpdate(_sz(orderbook['bids'][l]), _px(orderbook['bids'][l]),
                                    _px(orderbook['asks'][l]), _sz(orderbook['asks'][l]), time.time())

            orderbook = requests.get("http://api.hitbtc.com/api/1/public/BTCUSD/orderbook").json()
            self._depth_evt.put(("HB", _level(0), _level(1)))
            gevent.sleep(10)

    def depth_stream(self):
        evt = AsyncResult()
        gevent.spawn(self._depth_evt.get).link(evt.set)
        return evt

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


def mux(*args):
    evt = AsyncResult()
    for j in (gevent.spawn(lambda: a.wait()) for a in args):
        j.link(evt.set)
    return evt.get()


class Combiner(object):
    def __init__(self):
        self._storage = [None, None]
        pass

    def handle(self, name, named_mu):
        self._storage[0 if name == 'HB' else 1] = named_mu
        (mu0, mu1) = named_mu
        log.info("{} dt={:.6f}\t1={}\t2={}", name, time.time() - mu0.time, mu0, mu1)

        if self._storage[0 if name != 'HB' else 1] is None:
            return

        hb_b = self._storage[0][0].bid_px
        cs_b = self._storage[1][0].bid_px
        cs_exch_fee = .003
        b_sz = min(self._storage[0][0].bid_sz, self._storage[1][0].bid_sz)
        profit_b = b_sz * (-hb_b + (cs_b * (1 - cs_exch_fee)))
        if profit_b > 0:
            log.info("BUY HB={}\tSELL CS={}\tsz={}\tprofit={}", hb_b, cs_b, b_sz, profit_b)
            return

        hb_a = self._storage[0][0].ask_px
        cs_a = self._storage[1][0].ask_px
        a_sz = min(self._storage[0][0].ask_sz, self._storage[1][0].ask_sz)
        profit_a = a_sz * (+hb_a - (cs_a * (1 + cs_exch_fee)))
        if profit_a > 0:
            log.info("SELL HB={}\tBUY CS={}\tsz={}\tprofit={}", hb_a, cs_a, a_sz, profit_a)
            return

        log.info("Neither a:{} az:{}\tb:{} bz:{}", profit_a, a_sz, profit_b, b_sz)


class Mixer(object):
    @staticmethod
    def start():
        while True:
            e = mux(coinsetter_market_data.depth_stream(), hitbtc_market_data.depth_stream())
            print e

    def stop(self):
        pass


log = logbook.Logger(__name__)

if __name__ == "__main__":
    cmb = Combiner()
    cs_coalescer = Coalescer(functools.partial(cmb.handle, "CS"))
    hb_coalescer = Coalescer(functools.partial(cmb.handle, "HB"))

    coinsetter_market_data = Coinsetter()
    hitbtc_market_data = HitBtc()

    def mixer():
        while True:
            e = mux(coinsetter_market_data.depth_stream(), hitbtc_market_data.depth_stream())
            print e

    tasks = [coinsetter_market_data, hitbtc_market_data, mixer]

    try:
        gevent.joinall(map(gevent.spawn, (getattr(t, "start") for t in tasks)), raise_error=True)
    except Exception, e:
        for x in tasks:
            x.stop()
        log.exception("Unhandled exception during main loop", e)
        raise