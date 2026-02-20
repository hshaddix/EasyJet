

Diboson=('701085'
  '701090'
  '701095'
  '701100'
  '701105'
  '701110'
  '701115'
  '701120'
  '701125')

SingleTop=('601348' '601349' '601350' '601351' '601352' '601355')
Ttbar=('601229' '601230' '601237')
TtbarX=('522036' '522040' '602637' '602638' '700995' '700996')

Vjets=('700777'
  '700778'
  '700779'
  '700780'
  '700781'
  '700782'
  '700783'
  '700784'
  '700785'
  '700786'
  '700787'
  '700788'
  '700789'
  '700790'
  '700791'
  '700792'
  '700793'
  '700794'
  '700795'
  '700796'
  '700797')
 Vjets_qq=(
  '700843'
  '700849'
  '700855')

echo "Diboson"
for DSID in "${Diboson[@]}";
do
       # centralpage --scope=mc23_13p6TeV WeakBoson Diboson Baseline -p | grep ${DSID} 
       grep ${DSID} all_samples.txt | grep mc23a | awk '{print $5}' >> MC23a/Diboson_mc23a_p7017.txt 
       grep ${DSID} all_samples.txt | grep mc23d | awk '{print $5}' >> MC23d/Diboson_mc23d_p7017.txt
       grep ${DSID} all_samples.txt | grep mc23e | awk '{print $5}' >> MC23e/Diboson_mc23e_p7017.txt
done

echo "SingleTop"
for DSID in "${SingleTop[@]}";
do
       # centralpage --scope=mc23_13p6TeV Top SingleTop Baseline -p | grep ${DSID} >> SingleTop_mc23a_p7017.txt
       grep ${DSID} all_samples.txt | grep mc23a | awk '{print $5}' >> MC23a/SingleTop_mc23a_p7017.txt
       grep ${DSID} all_samples.txt | grep mc23d | awk '{print $5}' >> MC23d/SingleTop_mc23d_p7017.txt
       grep ${DSID} all_samples.txt | grep mc23e | awk '{print $5}' >> MC23e/SingleTop_mc23e_p7017.txt
done

echo "Ttbar"
for DSID in "${Ttbar[@]}";
do
       # centralpage --scope=mc23_13p6TeV Top TTbar Baseline -p | grep ${DSID}
       grep ${DSID} all_samples.txt | grep mc23a | awk '{print $5}' >> MC23a/Ttbar_mc23a_p7017.txt
       grep ${DSID} all_samples.txt | grep mc23d | awk '{print $5}' >> MC23d/Ttbar_mc23d_p7017.txt
       grep ${DSID} all_samples.txt | grep mc23e | awk '{print $5}' >> MC23e/Ttbar_mc23e_p7017.txt
done

echo "TtbarX"
for DSID in "${TtbarX[@]}";
do
       # centralpage --scope=mc23_13p6TeV Top TtbarX -p | grep ${DSID}
       grep ${DSID} all_samples.txt | grep mc23a | awk '{print $5}' >> MC23a/TtbarX_mc23a_p7017.txt
       grep ${DSID} all_samples.txt | grep mc23d | awk '{print $5}' >> MC23d/TtbarX_mc23d_p7017.txt
       grep ${DSID} all_samples.txt | grep mc23e | awk '{print $5}' >> MC23e/TtbarX_mc23e_p7017.txt
done

echo "Vjets"
for DSID in "${Vjets[@]}";
do
	#centralpage --scope=mc23_13p6TeV WeakBoson Vjets Baseline -p | grep ${DSID}
       grep ${DSID} all_samples.txt | grep mc23a | awk '{print $5}' >> MC23a/Vjets_mc23a_p7017.txt
       grep ${DSID} all_samples.txt | grep mc23d | awk '{print $5}' >> MC23d/Vjets_mc23d_p7017.txt
       grep ${DSID} all_samples.txt | grep mc23e | awk '{print $5}' >> MC23e/Vjets_mc23e_p7017.txt
done

for DSID in "${Vjets_qq[@]}";
do
      #  centralpage --scope=mc23_13p6TeV WeakBoson Vjets_qq Baseline -p | grep ${DSID}
      grep ${DSID} all_samples.txt | grep mc23a | awk '{print $5}' >> MC23a/Vjets_mc23a_p7017.txt
       grep ${DSID} all_samples.txt | grep mc23d | awk '{print $5}' >> MC23d/Vjets_mc23d_p7017.txt
       grep ${DSID} all_samples.txt | grep mc23e | awk '{print $5}' >> MC23e/Vjets_mc23e_p7017.txt
done
