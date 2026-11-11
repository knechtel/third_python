# CPython no maiOs

Esta pasta contém o port inicial do CPython 3.11.9 (`de54cf5`) para o maiOs.
O adaptador em `cpython_port.c` configura o runtime em modo isolado e oferece
funções para inicializar o interpretador, executar código e finalizar.

## Sysroot e compilação

O diretório `sysroot/usr/include` contém os headers freestanding mínimos usados
para configurar e compilar o core sem incluir headers da glibc do hospedeiro.
Eles descrevem a ABI e as funções atualmente esperadas pelo port; não são uma
implementação completa de libc. O alvo `i686-pc-maios` é reconhecido por `config.sub` e
`configure`, que seleciona stubs de thread e desativa recursos Unix indisponíveis.

Com um GCC que suporte `-m32`, a configuração local pode ser reproduzida assim
(o build fica em `/tmp`):

```sh
make -C third_party/cpython/ports/maios configure \
  CC='gcc -m32' \
  SYSROOT="$PWD/third_party/cpython/ports/maios/sysroot" \
  BUILD_DIR=/tmp/maios-cpython-build
make -C third_party/cpython/ports/maios core \
  BUILD_DIR=/tmp/maios-cpython-build
make -C third_party/cpython/ports/maios adapter \
  CC='gcc -m32' \
  SYSROOT="$PWD/third_party/cpython/ports/maios/sysroot" \
  BUILD_DIR=/tmp/maios-cpython-build
```

Para compilar o arquivo estático de stubs de runtime:

```sh
make -C third_party/cpython/ports/maios runtime \
  CC='gcc -m32' \
  SYSROOT="$PWD/third_party/cpython/ports/maios/sysroot" \
  BUILD_DIR=/tmp/maios-cpython-build
```

Em um toolchain dedicado, use `CC=i686-pc-maios-gcc`; `SYSROOT` é opcional se
ele já estiver embutido no compilador. O `pyconfig.h` produzido deve vir dessa
configuração do alvo. Não reutilize um `pyconfig.h` gerado contra os headers
Linux do host.

O port inclui `libm_dbl` do MicroPython vendorizado no repositório, baseado em
musl 1.1.16 sob licença MIT. `math` e `cmath` são built-ins e não dependem da
glibc. O módulo `time` continua desativado porque o kernel não tem relógio.
`encodings`, seus aliases e o codec UTF-8 ficam congelados no binário para o
bootstrap não depender de uma instalação Python no ext4.
`maiosfs` também é built-in. O runtime oferece stdio básico de console e
descritores de arquivo apoiados pelo ext4; a formatação `printf` cobre texto e
inteiros, ainda sem conversões de ponto flutuante.
O built-in `webserver` fornece `serve(handler, port=8070)`: o callback recebe
método, caminho e corpo e retorna texto ou bytes. Durante o callback,
`webserver.session()` retorna um dicionário persistente associado ao cookie
`maios_session`. Há até 16 sessões em memória; uma sessão antiga é substituída
quando o limite é alcançado. O cookie usa `HttpOnly` e `SameSite=Lax`. As
sessões desaparecem quando o processo termina, e `session()` fora do callback
gera `RuntimeError`. O servidor usa a API TCP do kernel, atende uma conexão
por vez e limita o corpo da resposta a 16 KiB.

## Runtime atual

`runtime/fs.c` adapta o console aos descritores 0–2 e usa ext4 para abrir,
ler, escrever e consultar arquivos no diretório atual. O port permite um
arquivo de leitura ou escrita aberto por vez, devido ao estado global da API
ext4. `runtime/posix.c`, `runtime/time.c`, `runtime/locale.c`,
`runtime/stdio.c`, `runtime/stdlib.c` e `runtime/cpython_compat.c` completam
os serviços básicos. Sinais e faulthandler ficam inativos; `PyOS_FSPath`
aceita `str`, `bytes` e objetos `PathLike`. Relógio e outras APIs ainda
indisponíveis retornam erro explícito.

O heap de 16 MiB em `freestanding.c` fornece `malloc`, `calloc`, `realloc` e
`free`. O comando `python` do shell inicializa o CPython uma vez e mantém o
interpretador entre execuções. `cpython_port.c` redireciona streams para
teclado/VGA, fornece `input()`, executa scripts do ext4 e oferece REPL de uma
linha. O kernel padrão é produzido como `kernel-cpython.elf`; para compilar
em outro caminho sem substituir a imagem atual, use
`make KERNEL_IMAGE=/tmp/maios-cpython.elf`.

O módulo `maiosfs` usa a API ext4 existente:

```python
import maiosfs

maiosfs.write("nota.bin", b"dados")
conteudo = maiosfs.read("nota.bin")
maiosfs.remove("nota.bin")
```

O alvo inicial é single-thread, sem signals, variáveis de ambiente, bytecode
gravado em disco ou importação automática de `site`.
