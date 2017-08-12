#include "K.h"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;namespace K {;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;void main(Local<Object> exports) {;;
;;;;;;EV::main(exports);;;;;;;;;;;;;;;;;
;;;;;;SD::main(exports);;    ;;;;    ;;;
;;;;;;UI::main(exports);;    ;;    ;;;;;
;;;;;;DB::main(exports);;        ;;;;;;;
;;;;;;QP::main(exports);;        ;;;;;;;
;;;;;;MG::main(exports);;    ;;    ;;;;;
;;;;;;OG::main(exports);;    ;;;;    ;;;
;;;;;;GW::main(exports);;    ;;;;    ;;;
;;;;};;;;;;;;;;;;;;;;;;;;;;;;;;;;    ;;;
;;};;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

NODE_MODULE(K, K::main)
