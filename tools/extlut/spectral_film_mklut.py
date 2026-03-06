import sys
import io
import warnings
import argparse
import json
import traceback
from contextlib import redirect_stdout, redirect_stderr

try:
    from spectral_film_lut.film_data import FilmData
    HAS_FILM_DATA = True
except ImportError:
    HAS_FILM_DATA = False


if HAS_FILM_DATA:
    films = [
        "Agfa Vista 100",
        "Fuji C200",
        "Fuji Eterna 500",
        "Fuji Eterna 500 Vivid",
        "Fuji FP-100C",
        "Fuji Instax color",
        "Fuji Natura 1600",
        "Fuji Pro 160C",
        "Fuji Pro 160S",
        "Fuji Pro 400H",
        "Fuji Provia 100F",
        "Fuji Superia Reala",
        "Fuji Superia X-Tra 400",
        "Fuji Velvia 50",
        "Kodachrome 64",
        "Kodak Vision3 250D 5207",
        "Kodak Vision3 500T 5219",
        "Kodak 5222 Dev 12",
        "Kodak 5222 Dev 4",
        "Kodak 5222 Dev 5",
        "Kodak 5222",
        "Kodak 5222 Dev 9",
        "Kodak 5247",
        "Kodak 5247 II",
        "Kodak 5248",
        "Kodak 5250",
        "Kodak Vision 320T 5277",
        "Kodak EXR 200T 5293",
        "Kodak Aerochrome III Infrared Film 1443",
        "Kodak Aerocolor IV 2460",
        "Kodak Aerocolor IV 2460 High",
        "Kodak Aerocolor IV 2460 Low",
        "KodakDyeTransferSeparation",
        "Kodak EXR 100T 5248",
        "Kodak Ektachrome 100D",
        "Kodak Ektar 100",
        "Kodak Gold 200",
        "Kodak Portra 160",
        "Kodak Portra 400",
        "Kodak Portra 800",
        "Kodak Portra 800 @1600",
        "Kodak Portra 800 @3200",
        "Kodak Trix-X 400 Dev 11",
        "Kodak Tri-X 400",
        "Kodak Trix-X 400 Dev 7",
        "Kodak Trix-X 400 Dev 9",
        "Kodak Ultramax 500",
        "Kodak Vericolor III",
        "Technicolor IV",
        "Technicolor IV Alt. 1",
        "Technicolor IV Alt. 2",
        "Kodak Vision3 50D 5203",
        "Kodak Vision3 200T 5213",
        "Kodak 5247 II Alt",
    ]
    papers = [
        "Fuji Eterna-CP Type 3513DI",
        "Fuji Crystal Archive DPII",
        "Fuji Crystal Archive Maxima",
        "Fuji Crystal Archive Super Type C",
        "Fujiflex Crystal Archive New Version",
        "Fujiflex Crystal Archive Old Version",
        "Kodak 2302 Dev 2",
        "Kodak 2302 Dev 3",
        "Kodak 2302 Dev 5",
        "Kodak 2302 Dev 7",
        "Kodak 2302 Dev 9",
        "Kodak Vision 2383",
        "Kodak Vision Premier 2393",
        "Kodak 5381",
        "Kodak 5383",
        "Kodak 5384",
        "Kodak Duraflex Plus",
        "KodakDyeTransferKodachrome",
        "KodakDyeTransferNegative",
        "Kodak Endura Premier Paper",
        "Kodak EXR 5386",
        "Kodak Professional Polymax Fine-Art Paper",
        "Kodak Polymax Fine-Art Paper Grade 0",
        "Kodak Polymax Fine-Art Paper Grade 1",
        "Kodak Polymax Fine-Art Paper Grade 2",
        "Kodak Polymax Fine-Art Paper Grade 3",
        "Kodak Polymax Fine-Art Paper Grade 4",
        "Kodak Polymax Fine-Art Paper Grade 5",
        "Kodak Polymax Fine-Art Paper Grade -1",
        "Kodak Portra Endura Paper",
        "Kodka Supra Endura Paper",
        "Technicolor V",
        "None",
        "Fuji Crystal Archive Pro PDII",
        "Ilfochrome Micrographic M",
        "Ilfochrome Micrographic P",
        "KodakDyeTransferSlide",
        "Kodak Ektachrome Radiance III Paper",
        "Fuji Eterna-CP 3523XD",
        "Kodak 2302",
    ]
else:
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
        from spectral_film_lut.utils import to_numpy
    except ImportError:
        def to_numpy(x): return x
        
    if HAS_FILM_DATA:
        import spectral_film_lut
        from spectral_film_lut.film_spectral import FilmSpectral
        def mkstock(s): return lambda : FilmSpectral(s)
        FILMSTOCKS = { s.name : mkstock(s)
                       for s in  spectral_film_lut.FILM_STOCKS }
    else:
        try:
            from spectral_film_lut import FILMSTOCKS
        except ImportError:
            import spectral_film_lut.film_loader
            FILMSTOCKS = {cls.__name__ : cls
                          for cls in spectral_film_lut.film_loader.filmstocks}
    FILMSTOCKS['None'] = lambda : None


def getopts():
    p = argparse.ArgumentParser()
    p.add_argument('--server', action='store_true')
    p.add_argument('--film', choices=films)
    p.add_argument('--paper', choices=papers)
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
    p.add_argument('--print-stocks-verbose', action='store_true')
    p.add_argument('params', nargs='?')
    p.add_argument('output', nargs='?')
    res = p.parse_args()
    if res.print_stocks_verbose:
        res.print_stocks = True
    return res


def print_stocks(verbose):
    out_films = []
    out_papers = []
    seen = set()
    for i, f in enumerate(films):
        seen.add(f)
        if f in FILMSTOCKS:
            out_films.append((f, i))
        else:
            out_films.append((f, -1))
    for i, p in enumerate(papers):
        seen.add(p)
        if p in FILMSTOCKS:
            out_papers.append((p, i))
        else:
            out_papers.append((p, -1))
    extra_films = []
    extra_papers = []
    for k in sorted(FILMSTOCKS):
        if k not in seen:
            if FILMSTOCKS[k]().stage == 'camera':
                extra_films.append((k, len(films) + len(extra_films)))
            else:
                extra_papers.append((k, len(papers) + len(extra_papers)))
    def sortkey(t):
        return t[1] < 0, t[0]
    out_films.sort(key=sortkey)
    out_papers.sort(key=sortkey)
    if verbose:
        for l in (out_films, out_papers, extra_films, extra_papers):
            for i, k in enumerate(l):
                if k[1] >= 0:
                    s = FILMSTOCKS[k[0]]()
                    if s is not None:
                        l[i] = (f'{k[0]} ({s.film_type.capitalize()})', k[1])
    def prl(l):
        return ',\n  '.join(f'["{k[0]}", {k[1]}]' for k in l)
    print(f'films = [\n  {prl(out_films)}\n]\n')
    print(f'papers = [\n  {prl(out_papers)}\n]\n')
    print(f'extra_films = [\n  {prl(extra_films)}\n]\n')
    print(f'extra_papers = [\n  {prl(extra_papers)}\n]\n')


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
    def default(): return None
    lut = create_lut(FILMSTOCKS.get(params['film'], default)(),
                     FILMSTOCKS.get(params['paper'], default)(),
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
    lut = to_numpy(lut)[...,:3]
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
        return print_stocks(opts.print_stocks_verbose)
    
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
