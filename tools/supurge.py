import asyncio, os, sys

from roborock.web_api import RoborockApiClient
from roborock.devices.device_manager import UserParams, create_device_manager
from roborock.roborock_typing import RoborockCommand

KOMUT = sys.argv[1] if len(sys.argv) > 1 else "durum"
V1 = {
    "baslat": RoborockCommand.APP_START,
    "durdur": RoborockCommand.APP_PAUSE,
    "sarj": RoborockCommand.APP_CHARGE,
    "bul": RoborockCommand.FIND_ME,
}


async def main():
    eposta = os.environ["ROBOROCK_EPOSTA"]
    sifre = os.environ["ROBOROCK_SIFRE"]
    istemci = RoborockApiClient(eposta)
    kullanici = await istemci.pass_login(sifre)
    yonetici = await create_device_manager(UserParams(username=eposta, user_data=kullanici))
    try:
        cihazlar = await yonetici.get_devices()
        if not cihazlar:
            print("HATA: hesapta cihaz yok")
            sys.exit(1)
        for c in cihazlar:
            pass
            if c.v1_properties is not None:
                if KOMUT == "durum":
                    await c.v1_properties.status.refresh()
                    print("Durum alindi")
                else:
                    await c.v1_properties.command.send(V1[KOMUT])
                    print("Gonderildi (v1):", KOMUT)
            elif c.b01_q7_properties is not None:
                q = c.b01_q7_properties
                if KOMUT == "baslat":
                    await q.start_clean()
                elif KOMUT == "durdur":
                    await q.pause_clean()
                elif KOMUT == "sarj":
                    await q.return_to_dock()
                elif KOMUT == "bul":
                    await q.find_me()
                else:
                    print("Bu modelde durum sorgusu henuz yok")
                print("Gonderildi (q7):", KOMUT)
            else:
                print("Desteklenmeyen model")
                sys.exit(1)
    finally:
        await yonetici.close()


asyncio.run(main())
