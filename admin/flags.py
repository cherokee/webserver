from Entry import *

ISO_3166 = {
    "AD" : "Andorra",
    "AE" : "United Arab Emirates",
    "AF" : "Afghanistan",
    "AG" : "Antigua and Barbuda",
    "AI" : "Anguilla",
    "AL" : "Albania",
    "AM" : "Armenia",
    "AN" : "Netherlands Antilles",
    "AO" : "Angola",
    "AR" : "Argentina",
    "AS" : "American Samoa",
    "AT" : "Austria",
    "AU" : "Australia",
    "AW" : "Aruba",
    "AX" : "Aland Islands",
    "AZ" : "Azerbaijan",
    "BA" : "Bosnia & Herzegovina",
    "BB" : "Barbados",
    "BD" : "Bangladesh",
    "BE" : "Belgium",
    "BF" : "Burkina Faso",
    "BG" : "Bulgaria",
    "BH" : "Bahrain",
    "BJ" : "Benin",
    "BM" : "Bermuda",
    "BN" : "Brunei Darussalam",
    "BO" : "Bolivia",
    "BR" : "Brazil",
    "BS" : "Bahamas",
    "BT" : "Bhutan",
    "BV" : "Bouvet Island",
    "BW" : "Botswana",
    "BY" : "Belarus",
    "BZ" : "Belize",
    "CA" : "Canada",
    "CC" : "Cocos (Keeling) Islands",
    "CD" : "Republic of the Congo",
    "CF" : "Central African Republic",
    "CG" : "Congo",
    "CH" : "Switzerland",
    "CI" : "Cote D'Ivoire",
    "CK" : "Cook Islands",
    "CL" : "Chile",
    "CM" : "Cameroon",
    "CN" : "China",
    "CO" : "Colombia",
    "CR" : "Costa Rica",
    "CU" : "Cuba",
    "CV" : "Cape Verde",
    "CX" : "Christmas Island",
    "CY" : "Cyprus",
    "CZ" : "Czech Republic",
    "DE" : "Germany",
    "DJ" : "Djibouti",
    "DK" : "Denmark",
    "DM" : "Dominica",
    "DO" : "Dominican Republic",
    "DZ" : "Algeria",
    "EC" : "Ecuador",
    "EE" : "Estonia",
    "EG" : "Egypt",
    "EH" : "Western Sahara",
    "ER" : "Eritrea",
    "ES" : "Spain",
    "ET" : "Ethiopia",
    "FI" : "Finland",
    "FJ" : "Fiji",
    "FK" : "Malvinas",
    "FM" : "Micronesia, States of",
    "FO" : "Faroe Islands",
    "FR" : "France",
    "GA" : "Gabon",
    "GB" : "United Kingdom",
    "GD" : "Grenada",
    "GE" : "Georgia",
    "GF" : "French Guiana",
    "GH" : "Ghana",
    "GI" : "Gibraltar",
    "GL" : "Greenland",
    "GM" : "Gambia",
    "GN" : "Guinea",
    "GP" : "Guadeloupe",
    "GQ" : "Equatorial Guinea",
    "GR" : "Greece",
    "GS" : "South Georgia",
    "GT" : "Guatemala",
    "GU" : "Guam",
    "GW" : "Guinea-Bissau",
    "GY" : "Guyana",
    "HK" : "Hong Kong",
    "HM" : "Heard & McDonald",
    "HN" : "Honduras",
    "HR" : "Croatia",
    "HT" : "Haiti",
    "HU" : "Hungary",
    "ID" : "Indonesia",
    "IE" : "Ireland",
    "IL" : "Israel",
    "IN" : "India",
    "IO" : "British Indian",
    "IQ" : "Iraq",
    "IR" : "Iran, Republic of",
    "IS" : "Iceland",
    "IT" : "Italy",
    "JM" : "Jamaica",
    "JO" : "Jordan",
    "JP" : "Japan",
    "KE" : "Kenya",
    "KG" : "Kyrgyzstan",
    "KH" : "Cambodia",
    "KI" : "Kiribati",
    "KM" : "Comoros",
    "KN" : "Saint Kitts and Nevis",
    "KP" : "Korea, Republic of",
    "KR" : "Korea, Republic of",
    "KW" : "Kuwait",
    "KY" : "Cayman Islands",
    "KZ" : "Kazakhstan",
    "LA" : "Lao, Republic",
    "LB" : "Lebanon",
    "LC" : "Saint Lucia",
    "LI" : "Liechtenstein",
    "LK" : "Sri Lanka",
    "LR" : "Liberia",
    "LS" : "Lesotho",
    "LT" : "Lithuania",
    "LU" : "Luxembourg",
    "LV" : "Latvia",
    "LY" : "Libyan Arab Jamahiriya",
    "MA" : "Morocco",
    "MC" : "Monaco",
    "MD" : "Moldova, Republic of",
    "ME" : "Montenegro",
    "MG" : "Madagascar",
    "MH" : "Marshall Islands",
    "MK" : "Macedonia",
    "ML" : "Mali",
    "MM" : "Myanmar",
    "MN" : "Mongolia",
    "MO" : "Macau",
    "MP" : "N. Mariana Islands",
    "MQ" : "Martinique",
    "MR" : "Mauritania",
    "MS" : "Montserrat",
    "MT" : "Malta",
    "MU" : "Mauritius",
    "MV" : "Maldives",
    "MW" : "Malawi",
    "MX" : "Mexico",
    "MY" : "Malaysia",
    "MZ" : "Mozambique",
    "NA" : "Namibia",
    "NC" : "New Caledonia",
    "NE" : "Niger",
    "NF" : "Norfolk Island",
    "NG" : "Nigeria",
    "NI" : "Nicaragua",
    "NL" : "Netherlands",
    "NO" : "Norway",
    "NP" : "Nepal",
    "NR" : "Nauru",
    "NU" : "Niue",
    "NZ" : "New Zealand",
    "OM" : "Oman",
    "PA" : "Panama",
    "PE" : "Peru",
    "PF" : "French Polynesia",
    "PG" : "Papua New Guinea",
    "PH" : "Philippines",
    "PK" : "Pakistan",
    "PL" : "Poland",
    "PM" : "S. Pierre & Miquelon",
    "PN" : "Pitcairn Islands",
    "PR" : "Puerto Rico",
    "PS" : "Palestinian Territory",
    "PT" : "Portugal",
    "PW" : "Palau",
    "PY" : "Paraguay",
    "QA" : "Qatar",
    "RE" : "Reunion",
    "RO" : "Romania",
    "RS" : "Serbia",
    "RU" : "Russian Federation",
    "RW" : "Rwanda",
    "SA" : "Saudi Arabia",
    "SB" : "Solomon Islands",
    "SC" : "Seychelles",
    "SD" : "Sudan",
    "SE" : "Sweden",
    "SG" : "Singapore",
    "SH" : "Saint Helena",
    "SI" : "Slovenia",
    "SJ" : "Svalbard and Jan Mayen",
    "SK" : "Slovakia",
    "SL" : "Sierra Leone",
    "SM" : "San Marino",
    "SN" : "Senegal",
    "SO" : "Somalia",
    "SR" : "Suriname",
    "ST" : "Sao Tome & Principe",
    "SV" : "El Salvador",
    "SY" : "Syrian Arab Republic",
    "SZ" : "Swaziland",
    "TC" : "Turks & Caicos Islands",
    "TD" : "Chad",
    "TF" : "French S. Territories",
    "TG" : "Togo",
    "TH" : "Thailand",
    "TJ" : "Tajikistan",
    "TK" : "Tokelau",
    "TL" : "Timor-Leste",
    "TM" : "Turkmenistan",
    "TN" : "Tunisia",
    "TO" : "Tonga",
    "TR" : "Turkey",
    "TT" : "Trinidad and Tobago",
    "TV" : "Tuvalu",
    "TW" : "Taiwan",
    "TZ" : "Tanzania, Republic of",
    "UA" : "Ukraine",
    "UG" : "Uganda",
    "UM" : "US Outlying Islands",
    "US" : "United States",
    "UY" : "Uruguay",
    "UZ" : "Uzbekistan",
    "VA" : "Vatican City State",
    "VC" : "S. Vincent & Grenadines",
    "VE" : "Venezuela",
    "VG" : "Virgin Islands, British",
    "VI" : "Virgin Islands, U.S.",
    "VN" : "Vietnam",
    "VU" : "Vanuatu",
    "WF" : "Wallis and Futuna",
    "WS" : "Samoa",
    "YE" : "Yemen",
    "YT" : "Mayotte",
    "ZA" : "South Africa",
    "ZM" : "Zambia",
    "ZW" : "Zimbabwe"
}

EXTRA_OPTIONS = [
    ("EU", "Europe", "europeanunion.png"),
    ("AP", "Asia/Pacific Region"),
    ("AQ", "Antarctica"),
    ("BI", "Burundi"),
    ("BL", "Saint Barthelemy"),
    ("FX", "France, Metropolitan"),
    ("GG", "Guernsey"),
    ("IM", "Isle of Man"),
    ("JE", "Jersey"),
    ("MF", "Saint Martin"),
    ("A1", "Anonymous Proxy"),
    ("A2", "Satellite Provider"),
    ("O1", "Other")
]

ADD_FLAGS_TO_KEY_JS = """
<script type="text/javascript">
function flags_add_to_key (options_id, cfg_key, cfg_key_value, url)
{
    var thisselect = get_by_id_and_class (options_id, 'select');
    var options    = thisselect.options;
    var selected   = options[options.selectedIndex].value;
    var value      = cfg_key_value + "," + selected;
    var post       = cfg_key + "=" + value;

    /* Do not add the country if it's already in the list
     */
    if (cfg_key_value.indexOf(selected) >= 0)
       return;

    jQuery.post (url, post, 
       function (data, textStatus) {
           window.location.reload();
       }
    );
}
</script>
"""


class OptionFlags:
    def __init__ (self, name, *args, **kwargs):
        self._name   = name
        self._kwargs = kwargs
        
    def _render_option (self, code, name, icon):
        if icon:
            style = 'style="background-image:url(/static/images/flags/%s); background-repeat:no-repeat; background-position:bottom right;" ' % (icon)
        else:
            style = ""

        return '\t<option %svalue="%s">%s - %s</option>\n' % (style, code, code, name)

    def __str__ (self):
        props = 'id="%s" name="%s"' % (self._name, self._name)

        for prop in self._kwargs:
            props += ' %s="%s"' % (prop, self._kwargs[prop])

        codes = ISO_3166.keys()
        codes.sort()

        txt = '<select %s>\n' % (props)
        txt += ' <optgroup label="Countries">'
        for code in codes:
            txt += self._render_option (code, ISO_3166[code], "%s.png"%(code))
        txt += ' </optgroup>'

        txt += ' <optgroup label="Extra">'
        for entry in EXTRA_OPTIONS:
            icon = None
            if len(entry) == 2:
                code, name = entry
            else:
                code, name, icon = entry
            txt += self._render_option (code, name, icon)
        txt += ' </optgroup>'
        txt += '</select>\n'

        return txt
