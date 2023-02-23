# IKPProject_VS
Potrebno je razviti rešenje za igru pogađanja zamišljenog broja. Sistem se sastoji iz dve osnovne komponente: server i neodređen broj igrača. 
Server treba prvo igračima da omogući registrovanje. Nakon registracije igrač može da zamisli broj pri čemu šalje serveru poruku koja sadrži interval u kom se zamišljeni broj nalazi. Server kada dobije poruku započinje igru i obaveštava ostale igrače da je igra počela. Igrači kreću da pogađaju broj tako što svoje predloge šalju serveru. Taj predlog server prosleđuje igraču koji je zamislio broj i on mu vraća odgovor ‘veće’, ‘manje’ ili ‘tačno’. Kada server dobije odgovor ‘tačno’ broj je pogođen i igra je gotova.
Pronaći algoritam pogađanja brojeva tako da je broj iteracija između igrača i servera što manji.
