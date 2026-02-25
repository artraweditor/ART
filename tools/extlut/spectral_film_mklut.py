import sys
import io
import warnings
import argparse
import json
import traceback
from contextlib import redirect_stdout, redirect_stderr

FIXED_STOCKS = True

if FIXED_STOCKS:
    films = [
        "AgfaVista100",
        "FujiC200",
        "FujiEterna500",
        "FujiEterna500Vivid",
        "FujiFP100C",
        "FujiInstaxColor",
        "FujiNatura1600",
        "FujiPro160C",
        "FujiPro160S",
        "FujiPro400H",
        "FujiProvia100F",
        "FujiSuperiaReala",
        "FujiSuperiaXtra400",
        "FujiVelvia50",
        "Kodachrome64",
        "Kodak5207",
        "Kodak5219",
        "Kodak5222Dev12",
        "Kodak5222Dev4",
        "Kodak5222Dev5",
        "Kodak5222Dev6",
        "Kodak5222Dev9",
        "Kodak5247",
        "Kodak5247II",
        "Kodak5248",
        "Kodak5250",
        "Kodak5277",
        "Kodak5293",
        "KodakAerochromeIII",
        "KodakAerocolor",
        "KodakAerocolorHigh",
        "KodakAerocolorLow",
        "KodakDyeTransferSeparation",
        "KodakEXR5248",
        "KodakEktachromeE100",
        "KodakEktar100",
        "KodakGold200",
        "KodakPortra160",
        "KodakPortra400",
        "KodakPortra800",
        "KodakPortra800At1600",
        "KodakPortra800At3200",
        "KodakTriX400Dev11",
        "KodakTriX400Dev6",
        "KodakTriX400Dev7",
        "KodakTriX400Dev9",
        "KodakUltramax400",
        "KodakVericolorIII",
        "TechnicolorIV",
        "TechnicolorIValt1",
        "TechnicolorIValt2",
        "Kodak5203",
        "Kodak5213"
    ]
    papers = [
        "Fuji3513DI",
        "FujiCrystalArchiveDPII",
        "FujiCrystalArchiveMaxima",
        "FujiCrystalArchiveSuperTypeC",
        "FujiflexNew",
        "FujiflexOld",
        "Kodak2303Dev2",
        "Kodak2303Dev3",
        "Kodak2303Dev5",
        "Kodak2303Dev7",
        "Kodak2303Dev9",
        "Kodak2383",
        "Kodak2393",
        "Kodak5381",
        "Kodak5383",
        "Kodak5384",
        "KodakDuraflexPlus",
        "KodakDyeTransferKodachrome",
        "KodakDyeTransferNegative",
        "KodakEnduraPremier",
        "KodakExr5386",
        "KodakPolymax",
        "KodakPolymaxGrade0",
        "KodakPolymaxGrade1",
        "KodakPolymaxGrade2",
        "KodakPolymaxGrade3",
        "KodakPolymaxGrade4",
        "KodakPolymaxGrade5",
        "KodakPolymaxGradeNeg1",
        "KodakPortraEndura",
        "KodakSupraEndura",
        "TechinicolorV",
        "None",
        "FujiCrystalArchiveProPDII",
        "IlfochromeMicrographicM",
        "IlfochromeMicrographicP",
        "KodakDyeTransferSlide",
        "KodakEktachromeRadianceIIIPaper"            
    ]

with warnings.catch_warnings(action='ignore'):
    from spectral_film_lut.utils import create_lut
    try:
        from spectral_film_lut import FILMSTOCKS
    except ImportError:
        import spectral_film_lut.film_loader
        FILMSTOCKS = {cls.__name__ : cls
                      for cls in spectral_film_lut.film_loader.filmstocks}
    if not FIXED_STOCKS:
        films = sorted(f for f in FILMSTOCKS
                       if FILMSTOCKS[f]().stage == 'camera')
        papers = sorted(f for f in FILMSTOCKS
                        if FILMSTOCKS[f]().stage == 'print') + ['None']
    FILMSTOCKS['None'] = lambda : None


def getopts():
    p = argparse.ArgumentParser()
    p.add_argument('--server', action='store_true')
    p.add_argument('--film', choices=films, default=films[0])
    p.add_argument('--paper', choices=papers, default=papers[0])
    p.add_argument('--exposure', type=float, default=0)
    p.add_argument('--wb', type=int, default=6500)
    p.add_argument('--tint', type=float, default=0)
    p.add_argument('--red-light', type=float, default=0)
    p.add_argument('--green-light', type=float, default=0)
    p.add_argument('--blue-light', type=float, default=0)
    p.add_argument('--projector-wb', type=int, default=6500)
    p.add_argument('--white-point', type=float, default=1)
    p.add_argument('--sat', type=float, default=1)
    p.add_argument('--black-offset', type=float, default=0)
    p.add_argument('--print-stocks', action='store_true')
    p.add_argument('params', nargs='?')
    p.add_argument('output', nargs='?')
    return p.parse_args()


def update_params(params, fname):
    with open(fname) as f:
        params.update(json.load(f))
        if isinstance(params['film'], int):
            params['film'] = films[params['film']]
        if isinstance(params['paper'], int):
            params['paper'] = papers[params['paper']]
    return params


def get_params(opts):
    params = {
        'film' : opts.film,
        'paper' : opts.paper,
        'exposure' : opts.exposure,
        'wb' : opts.wb,
        'tint' : opts.tint,
        'red_light' : opts.red_light,
        'green_light' : opts.green_light,
        'blue_light' : opts.blue_light,
        'projector_wb' : opts.projector_wb,
        'white_point' : opts.white_point,
        'sat' : opts.sat,
        'black_offset' : opts.black_offset,
    }
    if opts.params:
        params = update_params(params, opts.params)
    return params


def mklut(params, output):
    lut = create_lut(FILMSTOCKS[params['film']](),
                     FILMSTOCKS[params['paper']](),
                     lut_size=33,
                     cube=False,
                     input_colourspace='ACEScct',
                     output_colourspace='ACES2065-1',
                     projector_kelvin=params['projector_wb'],
                     exp_comp=params['exposure'],
                     white_point=params['white_point'],
                     exposure_kelvin=params['wb'],
                     mode='full',
                     red_light=params['red_light'],
                     green_light=params['green_light'],
                     blue_light=params['blue_light'],
                     black_offset=params['black_offset'],
                     color_masking=1,
                     tint=params['tint'],
                     sat_adjust=params['sat'],
                     matrix_method=False,
                     )
    dx, dy, dz = lut.shape[:-1]
    table = lut.reshape((-1, 3))
    with open(output, 'w') as out:
        out.write(f"""\
<?xml version="1.0" encoding="UTF-8"?>
<ProcessList compCLFversion="3" id="1">
    <Matrix inBitDepth="32f" outBitDepth="32f">
        <Array dim="3 3">
   1.45143931614567   -0.23651074689374  -0.214928569251925
-0.0765537733960206    1.17622969983357 -0.0996759264375522
0.00831614842569772 -0.00603244979102103    0.997716301365323
        </Array>
    </Matrix>
    <Log inBitDepth="32f" outBitDepth="32f" style="cameraLinToLog">
        <LogParams base="2" linSideSlope="1" linSideOffset="0" logSideSlope="0.0570776255707763" logSideOffset="0.554794520547945" linSideBreak="0.0078125" />
    </Log>
    <LUT3D inBitDepth="32f" outBitDepth="32f" interpolation="tetrahedral">
        <Array dim="{dx} {dy} {dz} 3">
""")
        for row in table:
            out.write(f'{row[0]:.7f} {row[1]:.7f} {row[2]:.7f}\n')
        out.write("""\
        </Array>
    </LUT3D>
</ProcessList>
""")


def main():
    opts = getopts()

    if opts.print_stocks:
        print(f'films = {json.dumps(films, indent=2)}')
        print(f'papers = {json.dumps(papers, indent=2)}')
        exit(0)
    
    params = get_params(opts)
    if opts.server:
        while True:
            p = sys.stdin.readline().strip()
            o = sys.stdin.readline().strip()
            buf = io.StringIO()
            try:
                params = update_params(params, p)
                with redirect_stdout(buf):
                    with redirect_stderr(buf):
                        mklut(params, o)
                        print(f'lut for {params} created in {o}')
                res = 'Y'
            except Exception:
                buf.write(traceback.format_exc())
                res = 'N'
            data = buf.getvalue().splitlines()
            sys.stdout.write(f'{res} {len(data)}\n')
            for line in data:
                sys.stdout.write(f'{line}\n')
            sys.stdout.flush()
    else:
        mklut(params, opts.output)
        

if __name__ == '__main__':
    with warnings.catch_warnings(action='ignore'):
        main()
